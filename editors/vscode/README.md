# Yuzu for VS Code

Syntax highlighting for the Yuzu language (`.yz` files).

The TextMate grammar in `syntaxes/yuzu.tmLanguage.json` is derived from the
compiler's token set (`yuzu/include/yuzu/Lexer/TokenKind.td`) and builtin type
names (`scalarBuiltins` in `yuzu/include/yuzu/Hir/Types/Type.h`): keywords,
operators, numeric/string literals, comments, builtin types, and `fn`
declarations / call sites.

## Try it (no install)

From this folder, run the *Extension Development Host*:

1. Open `editors/vscode/` in VS Code.
2. Press `F5` (Run → Start Debugging) — a second window opens with the
   extension loaded.
3. Open any `.yz` file (e.g. `yuzu/test/yuzu/**/*.yz`).

## Install locally

Copy or symlink this folder into your VS Code extensions directory:

```sh
ln -s "$PWD/editors/vscode" ~/.vscode/extensions/yuzu-0.1.0
# Or, if using dev containers.
# ln -s /workspaces/Yuzu/editors/vscode ~/.vscode-server/extensions/yuzu-0.1.0
```

then reload the window. To use it inside the dev container, add
`yuzu` to the devcontainer's `customizations.vscode.extensions` once it's
packaged/published, or mount it as above.

## Scope reference

Inspect a token's scope with **Developer: Inspect Editor Tokens and Scopes**
to see which `*.yuzu` scope a span resolved to (useful when tuning a theme or
extending the grammar).
