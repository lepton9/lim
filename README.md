# Lim

![Build](https://github.com/lepton9/lim/actions/workflows/build.yml/badge.svg)
![Commit](https://img.shields.io/github/last-commit/lepton9/lim)
![GitHub top language](https://img.shields.io/github/languages/top/lepton9/lim)
![Lines](https://tokei.rs/b1/github/lepton9/lim)

A basic Vim-like text editor.


## Commands

| Command               | Function                                        |
|-----------------------|-------------------------------------------------|
| `:w`                  | Write file                                      |
| `:q`                  | Quit                                            |
| `:help`               | Show help box                                   |
| `:/{string}`          | Search all matches in file for `string`         |
| `:r {string}`         | Replace all found matches with `string`         |
| `:r/{s1}/{s2}`        | Replace all matches for `s1` with `s2`          |
| `:%s/{s1}/{s2}`       | Replace all matches for `s1` with `s2`          |
| `:'<,'>s/{s1}/{s2}`   | Replace `s1` with `s2` after selection          |
| `:rename`             | Rename current file                             |
| `:restart`            | Restart editor                                  |
| `:set`                | Set config variable values                      |
| `:setconfig`          | Set path to `.limconfig`                        |
| `:configpath`         | Show config file path                           |
| `:path`               | Show path of current file or directory          |
| `:!`                  | Execute bash commands                           |

## Keymaps

| Keymap                | Function                                        |
|-----------------------|-------------------------------------------------|
| `h`, `j`, `k`, `l`    | Movement                                        |
| `gg`, `G`             | Go to beginning and end of the file             |
| `yy`, `yw`, `yiw`     | Yanking                                         |
| `dd`, `dw`, `diw`     | Deleting and yanking                            |
| `cw`, `ciw`           | Delete and change                               |
| `e`, `w`, `b`         | Inner word movement                             |
| `E`, `W`, `B`         | Movement over whitespace                        |
| `f{char}`, `F{char}`  | Find and go to the first occurrence of `char`   |
| `t{char}`, `T{char}`  | Go to before the first occurrence of `char`     |
| `r{char}`             | Replace the char under cursor with `char`       |
| `>>`                  | Shift to left                                   |
| `<<`                  | Shift to right                                  |
| `C+d`                 | Scroll down                                     |
| `C+u`                 | Scroll up                                       |
| `C+n`                 | Toggle file tree                                |
| `C+h`                 | Move to file tree if open                       |
| `C+l`                 | Move to file from file tree                     |
| `C+f`, `/`            | Search for matches                              |
| `C+p + #`             | Paste text from clipboard at index #            |
| `C+n`                 | Toggle file tree                                |

## Config file

```
// .limconfig

tab_indent = {int}
cursor_wrap = {true | false}
number = {true | false}
number_style = {normal | relative}
comline_visible = {true | false}
bg_color = 0xRRGGBB
fg_color = 0xRRGGBB
sb_bg_color = 0xRRGGBB
sb_fg_color = 0xRRGGBB
```
