#!/bin/bash
dpkg-buildpackage -us -uc -b

mv ../*.dsc ../*.deb ../*.ddeb ../*.tar.* ../*.buildinfo ../*.changes dist/



#debsign -k ABF8C9E8DF6D4AFD02BA58DCBA050865951ED7DD
