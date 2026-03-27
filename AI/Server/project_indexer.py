"""
project_indexer.py — Deep project file scanner for Atlas AI Server.

Refactored and adapted from Arbiter's archive_manager.py and library_manager.py.

Walks the entire MasterRepo project, builds a structured index of:
  - Directory tree
  - File inventory (path, size, language)
  - C++ symbol extraction (namespaces, classes, structs, functions)
  - Key documentation files (README, ROADMAP, etc.)
  - Summary stats

The index is injected as context into every Ollama prompt so the AI is
fully aware of the project structure without the user having to explain it.
"""

import os
import re
import json
import time
import hashlib
from pathlib import Path
from typing import Dict, List, Optional
from dataclasses import dataclass, field, asdict

# ── Directories to skip entirely ──────────────────────────────────────────────
SKIP_DIRS = {
    ".git", ".github", "Builds", "build", "Build", "bin", "lib",
    "node_modules", "__pycache__", ".venv", "venv", ".vs",
    "Archive",   # large graveyard — too noisy for context
    "External",  # third-party headers — no need to index
    "Logs",
    "CMakeFiles",
}

# ── File extensions to index ───────────────────────────────────────────────────
INDEX_EXTENSIONS = {
    ".h": "cpp", ".hpp": "cpp", ".cpp": "cpp", ".c": "c",
    ".py": "python", ".md": "markdown", ".json": "json",
    ".txt": "text", ".sh": "shell", ".bat": "batch",
    ".cmake": "cmake", ".toml": "toml", ".yaml": "yaml", ".yml": "yaml",
}

# ── Max sizes ──────────────────────────────────────────────────────────────────
MAX_FILE_SIZE_BYTES  = 256 * 1024   # 256 KB — skip enormous files
MAX_CONTEXT_CHARS    = 12_000       # chars of context injected per request
MAX_FILES_IN_INDEX   = 5_000       # cap total indexed files


@dataclass
class FileEntry:
    path: str           # relative to project root
    language: str
    size_bytes: int
    line_count: int
    symbols: List[str] = field(default_factory=list)   # classes/functions found


@dataclass
class ProjectIndex:
    root: str
    indexed_at: float
    total_files: int
    total_lines: int
    directory_tree: str          # ASCII tree (trimmed)
    file_entries: List[FileEntry]
    key_docs: Dict[str, str]     # filename → first 2000 chars
    summary: str                 # human-readable one-page summary


# ── Symbol extractor for C++ ───────────────────────────────────────────────────
_NS_RE    = re.compile(r'^\s*namespace\s+(\w+)', re.MULTILINE)
_CLASS_RE = re.compile(r'^\s*(?:class|struct)\s+(\w+)', re.MULTILINE)
_FN_RE    = re.compile(r'^\s*(?:virtual\s+|static\s+|inline\s+)?[\w:*&<>]+\s+(\w+)\s*\(', re.MULTILINE)

def _extract_cpp_symbols(text: str) -> List[str]:
    symbols: List[str] = []
    for m in _NS_RE.finditer(text):
        symbols.append(f"namespace:{m.group(1)}")
    for m in _CLASS_RE.finditer(text):
        symbols.append(f"class:{m.group(1)}")
    fns = [m.group(1) for m in _FN_RE.finditer(text)]
    # Deduplicate, keep first 12 functions
    seen = set()
    for fn in fns[:12]:
        if fn not in seen and fn not in ("if", "while", "for", "switch", "return"):
            symbols.append(f"fn:{fn}")
            seen.add(fn)
    return symbols


# ── Directory tree builder ─────────────────────────────────────────────────────
def _build_tree(root: Path, prefix: str = "", depth: int = 0, max_depth: int = 4) -> str:
    if depth > max_depth:
        return ""
    lines: List[str] = []
    try:
        entries = sorted(root.iterdir(), key=lambda p: (p.is_file(), p.name.lower()))
    except PermissionError:
        return ""

    visible = [e for e in entries if e.name not in SKIP_DIRS and not e.name.startswith(".")]
    for i, entry in enumerate(visible):
        connector = "└── " if i == len(visible) - 1 else "├── "
        lines.append(prefix + connector + entry.name)
        if entry.is_dir():
            extension = "    " if i == len(visible) - 1 else "│   "
            sub = _build_tree(entry, prefix + extension, depth + 1, max_depth)
            if sub:
                lines.append(sub)
    return "\n".join(lines)


# ── Main indexer ───────────────────────────────────────────────────────────────
class ProjectIndexer:
    def __init__(self, project_root: str):
        self.root = Path(project_root).resolve()
        self._index: Optional[ProjectIndex] = None
        self._index_hash: str = ""

    # ── Public API ─────────────────────────────────────────────────────────────
    def index(self, force: bool = False) -> ProjectIndex:
        """Build (or rebuild) the full project index."""
        print(f"[Atlas AI] Indexing project: {self.root}")
        t0 = time.time()

        file_entries: List[FileEntry] = []
        key_docs: Dict[str, str] = {}
        total_lines = 0
        file_count  = 0

        for dirpath, dirnames, filenames in os.walk(str(self.root)):
            # Prune skip dirs in-place so os.walk won't descend
            dirnames[:] = [d for d in dirnames
                           if d not in SKIP_DIRS and not d.startswith(".")]
            dirnames.sort()

            for fname in sorted(filenames):
                if file_count >= MAX_FILES_IN_INDEX:
                    break
                fpath = Path(dirpath) / fname
                ext   = fpath.suffix.lower()
                if ext not in INDEX_EXTENSIONS:
                    continue

                try:
                    size = fpath.stat().st_size
                except OSError:
                    continue
                if size > MAX_FILE_SIZE_BYTES:
                    continue

                try:
                    text = fpath.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    continue

                lines      = text.count("\n") + 1
                total_lines += lines
                lang       = INDEX_EXTENSIONS[ext]
                rel_path   = str(fpath.relative_to(self.root)).replace("\\", "/")

                symbols: List[str] = []
                if lang == "cpp":
                    symbols = _extract_cpp_symbols(text)

                file_entries.append(FileEntry(
                    path=rel_path,
                    language=lang,
                    size_bytes=size,
                    line_count=lines,
                    symbols=symbols,
                ))

                # Capture key documentation files in full
                if fname in ("README.md", "ROADMAP.md", "AUDIT.md") and lang == "markdown":
                    key_docs[fname] = text[:3000]

                file_count += 1

        tree = _build_tree(self.root, max_depth=3)
        summary = self._build_summary(file_entries, total_lines)

        idx = ProjectIndex(
            root=str(self.root),
            indexed_at=time.time(),
            total_files=file_count,
            total_lines=total_lines,
            directory_tree=tree,
            file_entries=file_entries,
            key_docs=key_docs,
            summary=summary,
        )
        self._index = idx
        print(f"[Atlas AI] Index complete: {file_count} files, {total_lines:,} lines "
              f"in {time.time() - t0:.1f}s")
        return idx

    def get_index(self) -> Optional[ProjectIndex]:
        return self._index

    def is_ready(self) -> bool:
        return self._index is not None

    def build_context(self, query: str = "", max_chars: int = MAX_CONTEXT_CHARS) -> str:
        """Build a focused context string to inject into every AI prompt."""
        if not self._index:
            return "Project index not yet available."

        idx = self._index
        parts: List[str] = []

        # 1. High-level summary (always included)
        parts.append(f"=== MasterRepo Project Context ===\n{idx.summary}\n")

        # 2. Directory tree (trimmed)
        tree_lines = idx.directory_tree.split("\n")[:60]
        parts.append("=== Directory Structure (top 3 levels) ===\n"
                     + "\n".join(tree_lines) + "\n")

        # 3. README excerpt
        if "README.md" in idx.key_docs:
            parts.append("=== README.md (excerpt) ===\n"
                         + idx.key_docs["README.md"][:1200] + "\n")

        # 4. Query-relevant file symbols
        if query:
            ql = query.lower()
            relevant = [
                f for f in idx.file_entries
                if ql in f.path.lower() or
                   any(ql in s.lower() for s in f.symbols)
            ][:8]
            if relevant:
                parts.append("=== Relevant Files ===")
                for fe in relevant:
                    syms = ", ".join(fe.symbols[:6]) if fe.symbols else "—"
                    parts.append(f"  {fe.path}  ({fe.language}, {fe.line_count} lines)  [{syms}]")
                parts.append("")

        ctx = "\n".join(parts)
        return ctx[:max_chars]

    def read_file(self, rel_path: str) -> str:
        """Read a project file and return its content (safety-checked)."""
        target = (self.root / rel_path).resolve()
        # Prevent path traversal outside project root
        if not str(target).startswith(str(self.root)):
            return "Access denied: path outside project root."
        if not target.is_file():
            return f"File not found: {rel_path}"
        try:
            size = target.stat().st_size
            if size > MAX_FILE_SIZE_BYTES:
                return f"File too large to read ({size // 1024} KB). Use a smaller section."
            return target.read_text(encoding="utf-8", errors="replace")
        except OSError as e:
            return f"Error reading file: {e}"

    def list_files(self, subdir: str = "") -> List[Dict]:
        """List files under a subdirectory."""
        target = (self.root / subdir).resolve() if subdir else self.root
        if not str(target).startswith(str(self.root)):
            return []
        if not target.is_dir():
            return []
        results = []
        try:
            for entry in sorted(target.iterdir(), key=lambda p: (p.is_file(), p.name)):
                if entry.name.startswith(".") or entry.name in SKIP_DIRS:
                    continue
                results.append({
                    "name":     entry.name,
                    "path":     str(entry.relative_to(self.root)).replace("\\", "/"),
                    "is_dir":   entry.is_dir(),
                    "size":     entry.stat().st_size if entry.is_file() else 0,
                    "language": INDEX_EXTENSIONS.get(entry.suffix.lower(), "") if entry.is_file() else "",
                })
        except PermissionError:
            pass
        return results

    # ── Internals ──────────────────────────────────────────────────────────────
    def _build_summary(self, entries: List[FileEntry], total_lines: int) -> str:
        lang_counts: Dict[str, int] = {}
        for fe in entries:
            lang_counts[fe.language] = lang_counts.get(fe.language, 0) + 1

        lang_str = ", ".join(
            f"{lang}:{count}" for lang, count in
            sorted(lang_counts.items(), key=lambda x: -x[1])
        )

        top_dirs: Dict[str, int] = {}
        for fe in entries:
            top = fe.path.split("/")[0]
            top_dirs[top] = top_dirs.get(top, 0) + 1

        top_dir_str = ", ".join(
            f"{d}({n})" for d, n in
            sorted(top_dirs.items(), key=lambda x: -x[1])[:12]
        )

        return (
            f"Project: MasterRepo — Offline AI-powered game engine + editor + game\n"
            f"Root: {self.root}\n"
            f"Files indexed: {len(entries):,}  |  Total lines: {total_lines:,}\n"
            f"Languages: {lang_str}\n"
            f"Top directories: {top_dir_str}\n\n"
            f"Key subsystems:\n"
            f"  Engine/    — OpenGL rendering, physics, audio, input\n"
            f"  Editor/    — Custom UI editor (no ImGui), viewport, gizmos, ECS outliner\n"
            f"  Runtime/   — ECS world, player, gameplay, AI miners, inventory\n"
            f"  AI/        — Ollama client, agents, project indexer (this system)\n"
            f"  AI/Server/ — Atlas AI local web server (FastAPI, port 8080)\n"
            f"  Projects/NovaForge/ — 3D space game using the engine\n"
            f"  Builder/   — Starbase-style module builder\n"
            f"  PCG/       — Procedural content generation\n"
            f"  IDE/       — Code editor, AI chat panel\n"
            f"  Core/      — Events, serialisation, logging, ECS\n"
        )
