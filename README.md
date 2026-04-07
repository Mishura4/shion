# shion
Homemade C++ library with various tools I use across projects

## Troubleshooting
### Ambiguous module std
If you are using CLion and want to use shion's `import std;` feature, you may want to disable ReSharper's std module search, as you will get ambiguous symbols for any std symbol. Go to the registry (`Help | Find Action...`, type `Registry` there), set `cidr.compiler.info.std.modules.search` to false, then invalidate your ReSharper caches and restart your IDE.
