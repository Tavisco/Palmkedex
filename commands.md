## Crush the PNG
```bash
pngcrush -ow -fix -force -nofilecheck -brute -rem alla -oldtimestamp 252.png
```
## Generate APP Color Icon from PNG
``` bash
convert poke.png -gravity Center -crop 22x22+0+0 -filter point -background '#00FF00' -alpha remove -depth 8 -type palette BMP3:poke-8.bmp
```

## Generate APP Greysacale Icon from PNG
``` bash
convert poke.png -colorspace Gray -ordered-dither checks -gravity Center -crop 22x22+0+0 -filter point -background white -alpha remove -depth 1 -type palette BMP3:poke-1.bmp
```

## Small icon
``` bash
convert icon-lg-1.bmp -ordered-dither checks -filter box -resize 15x9 -gravity center -background white -extent 15x9 -depth 1 -type palette BMP3:icon-sm-1.bmp
```

## High Res Icon
``` bash
convert poke.png -magnify -gravity Center -crop 44x44+0+0  -background '#00FF00' -alpha remove -depth 8 -type palette BMP3:poke-8-d144.bmp
```


## Format Pokémon sprites in bulk to BMP
### 8 BPP 
``` bash
for file in *.png; do convert "$file" -background white -alpha remove -depth 8 -type palette BMP3:"`basename \"$file\" .png`"-8.bmp; done
```

### 4 BPP
``` bash
for file in *.png; do convert "$file" -background white -alpha remove -colorspace gray -depth 4 -type palette BMP3:"`basename \"$file\" .png`"-4.bmp; done
```


## Type icons
``` bash
convert BugIC_Colo.png -background white +dither -filter Mitchell -define filter:window=Jinc -define filter:lobes=2 -resize 64x -background white -alpha remove -depth 8 -colors 256 -type palette -compress None BMP3:out.bmp

convert grass.png -background white -dither FloydSteinberg -filter point -resize 32x -gravity center -background white -alpha remove -colorspace gray -depth 4 -type palette -compress None BMP3:out.bmp
 ```

## Git Patches
``` bash
git apply --reject --whitespace=fix

wiggle --replace Src/pngDraw.h Src/pngDraw.h.rej
```
for file in *.png; do convert "$file" -background white +dither -filter Mitchell -define filter:window=Jinc -define filter:lobes=3 -resize 100x -background white -alpha remove -depth 8 -colors 256 -type palette -compress None BMP3:"`basename \"$file\" .png`"-hr.bmp; done


``` bash
for file in *-icn.bmp; do convert "$file" -fuzz 10% -fill white -opaque "#00FF00" -dither FloydSteinberg -filter point -colorspace gray -depth 1 -type palette -compress None BMP3:"`basename \"$file\" .bmp`"-1.bmp; done
```


