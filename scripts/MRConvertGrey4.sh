rm -rf out
mkdir out
for i in hres/*.png
do
	base=$(echo "$i" | sed 's/.*\/0*\([1-9][0-9]*\)\..*/\1/g')
	
	convert $i -resize 96x96 -dither FloydSteinberg -colors 16 -colorspace gray -type truecolor tmp.bmp
	
	dst=$(printf "out/pSPT%04x.bin" $base)
	./compress c4 < tmp.bmp > $dst
	echo $i
done
/mnt/hgfs/D/projects/arm/rePalm/tools/mkrom/mkprc "PalmkedexSprites" pSPR PKSP out/* > sprites.new.prc