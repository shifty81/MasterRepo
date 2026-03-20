# Training Data — Error/Fix Pairs

Compiler errors, runtime errors, and their corresponding fixes. Each pair is a
JSON file containing the original broken code, the error message, and the
corrected code. Used by `FixAgent` for few-shot learning of common bug
patterns in this codebase.
