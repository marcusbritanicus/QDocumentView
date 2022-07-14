# QDocumentView

A simple widget to display paged documents, like PDF, DjVu and so on.. Currently, PDF and DjVu are natively supported.

## Features
* Page-rotations: 0, 90, 180, 270
* Page Layout: Single, Facing, Book
* Search highlight (if supported by the backend)
* Custom zoom in the range 10% to 400%


### Notes for compiling (Qt5) - linux:

- Install all the dependencies
- Download the sources
  * Git: `git clone https://gitlab.com/marcusbritanicus/qdocumentview.git qdocumentview`
- Enter the `qdocumentview` folder
  * `cd qdocumentview`
- Configure the project - we use meson for project management
  * `meson .build --prefix=/usr --buildtype=release`
- Compile and install - we use ninja
  * `ninja -C .build -k 0 -j $(nproc) && sudo ninja -C .build install`

### Dependencies:
#### Main library
* Qt5 (qtbase5-dev)

#### Backends
* Poppler Qt5 (libpoppler-qt5-dev)
* DjVULibre (libdjvulibre-dev)

### Upcoming
* Plugins to support various document formats, like ps, cbr, cbz, etc
* Any other feature you request for... :)
