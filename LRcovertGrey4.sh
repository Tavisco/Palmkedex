for i in src/*.png
do
	base=$(echo "$i" | sed 's/.*\/0*\([1-9][0-9]*\)\..*/\1/g')
	
	convert $i -dither FloydSteinberg -colors 16 -colorspace gray -normalize -type truecolor tmp.bmp
	
	dst=$(printf "out/pSPT%04x.bin" $base)
	./compress c4 < tmp.bmp > $dst
	echo $i
done
/mnt/hgfs/D/projects/arm/rePalm/tools/mkrom/mkprc "PalmkedexSprites" pSPR PKSP out/* > sprites.new.prc