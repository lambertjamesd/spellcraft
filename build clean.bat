docker run -v .\\:/spellcraft -it --rm dockerhub.lambertjamesd.com/spellcraft make build/assets/scripts/globals.dat
docker run -v .\\:/spellcraft -it --rm dockerhub.lambertjamesd.com/spellcraft make clean
docker run -v .\\:/spellcraft -it --rm dockerhub.lambertjamesd.com/spellcraft make
pause