# Luin v2.4

A small programming language (interpreter + optional .sxc compiler).

## Quick start (Linux / macOS)

```bash
chmod +x Luin_v2.4
./Luin_v2.4 test.sx
```

Or rebuild from source:

```bash
./build.sh          # produces ./Luin_v2.4
./Luin_v2.4 test.sx
```

## Compile a script

```bash
./Luin_v2.4 myprogram.sx -b     # writes myprogram.sxc
./Luin_v2.4 myprogram.sxc -r    # run the compiled form
```

## Version checker

On every launch Luin checks https://xluin.netlify.app/version.txt
(at most once per hour). If a newer version is published, an update
notice is printed to stderr.

Remote version.txt format (publish this on your Netlify site):

```
2.5
Your changelog / news lines here.
Please download from https://xluin.netlify.app
```

- First non-empty line = version number (optional leading "v")
- Remaining lines = news text shown in the update banner
- State is stored in ~/.luin/version_state.json

## Modules

import math / time / string / random / arrays / os / app
import "helpers.sx"

See CHANGES_v2.2.md, CHANGES_v2.3.md, CHANGES_v2.4.md for full history.
