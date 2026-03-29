# linAIxOS System Libraries

These are the core system libraries of linAIxOS. Where functionality isn't expected in the C standard library, these provide additional features that are shared by multiple linAIxOS applications.

## `linAIx_auth`

Provides password validation and login helper methods. Exists primarily because `libc` doesn't have these things and there are multiple places where logins are checked (`login`, `glogin`, `sudo`, `gsudo`...).

## `linAIx_button`

Renderer for button widgets. Not really a widget library at the moment.

## `linAIx_confreader`

Implements a basic INI parser for use with configuration files.

## `linAIx_decorations`

Client-side decoration library for the compositor. Supports pluggable decoration themes through additional libraries, which are named as `liblinAIx_decor-...`.

## `linAIx_graphics`

General-purpose 2D drawing and pixel-pushing library. Provides sprite blitting, rotation, scaling, etc.

## `linAIx_hashmap`

Generic hashmap implementation. Also used by the kernel.

## `linAIx_iconcache`

Convenience library for loading icons at specific sizes.

## `linAIx_inflate`

Decompression library for DEFLATE payloads.

## `linAIx_jpeg`

Minimal, incomplete JPEG decoder. Mostly used for providing wallpapers. Doesn't support most JPEG features.

## `linAIx_kbd`

Keyboard scancode parser.

## `linAIx_list`

Generic expandable linked list implementation.

## `linAIx_markup`

XML-like syntax parser.

## `linAIx_menu`

Menu widget library. Used for the "Applications" menu, context menus, etc.

## `linAIx_pex`

Userspace library for using the linAIxOS "packetfs" subsystem, which provides packet-based IPC.

## `linAIx_png`

Decoder for Portable Network Graphics images.

## `linAIx_rline`

Rich line editor for terminal applications, with support for tab completion and syntax highlighting.

## `linAIx_termemu`

Terminal ANSI escape processor.

## `linAIx_text`

TrueType font parser and text renderer.

## `linAIx_tree`

Generic tree implementation. Also used by the kernel.

## `linAIx_yutani`

Compositor client library, used to build GUI applications.

