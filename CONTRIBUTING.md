# Contributing to linux-g13-driver

The project is completed: it is mainly relevant for kernels older than 6.19. For current kernels
use the successor [gkey-config](https://github.com/Lordbooker/gkey-config). Bug fixes are still
welcome; please open an [issue](https://github.com/Lordbooker/linux-g13-driver/issues) first.

## Branches

- `dev` – development; all work happens here, pushes build test packages.
- `main` – released state; updated from `dev` by pull request, releases are tagged here.

Pull requests go against `dev`. Code, comments, commit messages and pull requests are in English.

## Building

Dependencies: see the README section *Requirements*, or run `make deps` in `g13-driver/`.

```bash
cmake -S g13-driver -B build -DCMAKE_BUILD_TYPE=Debug -DG13_WERROR=ON
cmake --build build
```

| CMake option | Default | Meaning |
|---|---|---|
| `BUILD_CONFIG_TOOL` | `ON` | build the Qt6 config tool `g13-gui` |
| `ENABLE_SANITIZERS` | `OFF` | ASan/UBSan, only for `CMAKE_BUILD_TYPE=Debug` |
| `G13_WERROR` | `OFF` | compiler warnings are errors (CI) |
| `G13_VERSION` | `git describe` | displayed version |
| `G13_PACKAGE_VERSION`, `G13_PACKAGE_RELEASE` | from the version, `1` | package version and release for CPack |

The code is C++23 (daemon: standard library, libusb, uinput; config tool: Qt 6). Errors are
logged to syslog/journal and handled as return values; the daemon does not use exceptions for
control flow.

## Checks

The workflow *Build* (`.github/workflows/build.yml`) runs on every push to `dev`/`main` and on
pull requests:

- build on Arch Linux, Fedora and Debian testing with `-Werror`, check the udev rule
  (`udevadm verify`) and the desktop files, test install into a staging directory,
- static analysis: `clang-tidy` with the configuration in `.clang-tidy`, `cppcheck`,
  `shellcheck` and `actionlint`.

The same checks locally:

```bash
CXX=clang++ cmake -S g13-driver -B build -DCMAKE_BUILD_TYPE=Debug -DG13_WERROR=ON
cmake --build build
run-clang-tidy -quiet -p build -warnings-as-errors='*' g13-driver/src/driver/*.cpp g13-driver/src/gui/*.cpp
cppcheck --quiet --inline-suppr --std=c++20 --enable=warning,style,performance,portability \
  --suppress=missingIncludeSystem -I g13-driver/src/driver g13-driver/src/driver
shellcheck -S warning g13-driver/scripts/*.sh g13-driver/packaging/build-package.sh
actionlint
```

## Translations

The config tool follows the system language; translations are built into the binary. After
text changes in the code, update the `.ts` files with
`cmake --build build --target update_translations` and translate them with Qt Linguist.

## Packages

`g13-driver/packaging/build-package.sh` builds the `.deb` or `.rpm` inside a container of the
target distribution, the same way the release workflow does:

```bash
docker run --rm -v "$PWD:/src" -w /src debian:13 g13-driver/packaging/build-package.sh dist
docker run --rm -v "$PWD:/src" -w /src fedora:44 g13-driver/packaging/build-package.sh dist
```

The AUR package `linux-g13-driver-git` is in `g13-driver/packaging/arch/`; on every push to `dev`
the workflow builds it from the pushed commit as a test.

## Releases

1. Set the version in `project(Linux-G13-Driver VERSION x.y.z)` in `g13-driver/CMakeLists.txt`
   (on `dev`).
2. Merge `dev` into `main` by pull request.
3. Tag `main` and push the tag:

   ```bash
   git switch main && git pull
   git tag -a vx.y.z -m "linux-g13-driver x.y.z"
   git push origin vx.y.z
   git push origin main:dev   # dev contains the tag again
   ```

   The last line only fast-forwards `dev` to the merge commit, so that development builds show
   versions like `x.y.z-3-gabc1234`.

4. The workflow *Release packages* checks that the tag matches the project version, builds the
   packages, creates the GitHub release and publishes the AUR package `linux-g13-driver-git`.
   Publishing to the AUR needs the repository secret `AUR_SSH_PRIVATE_KEY`
   (*Settings → Secrets and variables → Actions → Repository secrets*): the complete private SSH
   key of the AUR account without passphrase, including the `-----BEGIN`/`-----END` lines.
   Without it the AUR step only shows a notice and the files are attached to the run as artifact
   `aur`; after adding the secret, *Re-run all jobs* publishes them (existing release assets are
   replaced).
