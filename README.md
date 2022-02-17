# QDocumentView

A simple widget to display paged documents, like PDF, DjVu and so on..

## Features
* Page-rotations: 0, 90, 180, 360
* Page Layout: Single, Facing, Book
* Search highlight (if supported by the backend)

### Notes for compiling (Qt5) - linux:

* Download the sources
  * Git: `git clone https://gitlab.com/marcusbritanicus/QDocumentView.git QDocumentView-master`
* Enter `QDocumentView-master`, and create & enter the build directory.
* Open the terminal and type: `cd QDocumentView-master && mkdir .build; cd .build`
* Configure and compile the project: `cmake .. && make -kj$(nproc)`
* To install, as root type: `make install`

### Dependencies:
#### Main library
* Qt5 (qtbase5-dev)

#### Backends
* QtPdf (qtpdf-dev)
* Poppler Qt5 (libpoppler-qt5-dev)
* DjVULibre (libdjvulibre-dev)

## My System Info
* OS:				Debian Sid
* Qt:				5.15.2
* QtPdf:            5.15.6
* Poppler:          20.09.0
* DjVu Libre        3.5.28

### Upcoming
* Any other feature you request for... :)
