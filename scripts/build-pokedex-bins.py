from ctypes import sizeof
from locale import bind_textdomain_codeset
import pokebase as pb
import unicodedata
import json
import struct
import subprocess
from pathlib import Path

PkmnTypes = {1: 'normal', 2: 'fire', 3: 'water', 4: 'grass', 5: 'electric', 6: 'rock', 7: 'ground', 8: 'ice', 9: 'flying', 10: 'fighting', 11: 'ghost',
                12: 'bug', 13: 'poison', 14: 'psychic', 15: 'steel', 16: 'dark', 17: 'dragon', 18: 'fairy', 19: 'unknown', 20: 'shadow', 21: 'none'}

def rgb_to_hex(red, green, blue):
    """Return color as #rrggbb for the given color values."""
    return '#%02x%02x%02x' % (int(red), int(green), int(blue))

def word_to_hex(word):
    enc = word.encode("ascii")
    return enc.hex()

def get_type(pkmnType) -> int:
    for count, item in enumerate(PkmnTypes, start=1):
        if PkmnTypes[item] == pkmnType:
            return count
        
def strip_accents(text):
    """
    Strip accents from input String.

    :param text: The input string.
    :type text: String.

    :returns: The processed String.
    :rtype: String.
    """
    try:
        text = unicode(text, 'utf-8')
    except (TypeError, NameError): # unicode is a default on python 3 
        pass
    text = unicodedata.normalize('NFKD', text)
    text = text.encode('ascii', 'ignore')
    text = text.decode("utf-8")
    return str(text)

def getFlavor(entries):
    for entry in entries:
        if entry.language.name == "en":
            return entry.flavor_text

if __name__=="__main__":
    # 905 pokemons
    pkmnQuantity = 905

    rsrcStr = ""
    rsrcImgStr = ""
    pkmnNames = "#define PKMN_QUANTITY = " + str(pkmnQuantity) + "\n"
    
    for i in range(1, pkmnQuantity+1):
        pkmn =  pb.pokemon(i)
        spc = pb.pokemon_species(i)
        pkmnDesc = getFlavor(spc.flavor_text_entries).replace(u'\f',       u'\n') \
                          .replace(u'\u00ad\n', u'') \
                          .replace(u'\u00ad',   u'') \
                          .replace(u' -\n',     u' - ') \
                          .replace(u'-\n',      u'-') \
                          .replace(u'\n',       u' ')
        pkmnDesc = strip_accents(pkmnDesc)
        pkmnDesc = pkmnDesc.replace("POKeMON", "POKEMON")
        pkmnDesc += "\0"

        print("#" + str(i) + " - " + pkmn.name + ": " + pkmnDesc)

        b = bytes([
            pkmn.stats[0].base_stat,                                            # HP
            pkmn.stats[1].base_stat,                                            # Attack
            pkmn.stats[2].base_stat,                                            # Defense
            pkmn.stats[3].base_stat,                                            # Special-attack
            pkmn.stats[4].base_stat,                                            # Special-defense
            pkmn.stats[5].base_stat,                                            # speed
            get_type(pkmn.types[0].type.name),                                  # Type 1
            get_type(pkmn.types[1].type.name) if len(pkmn.types) > 1 else 21    # Type 2
        ])

        indexStr = str(i).rjust(4, '0')
        infoFilename = "pINF" + indexStr + ".bin"
        descFilename = "pDSC" + indexStr + ".bin"

        with open("bin/" + infoFilename, "wb") as file:
            file.write(b)

        with open("bin/" + descFilename, "wb") as file:
            file.write(pkmnDesc.encode('utf-8'))
        
        rsrcStr += "DATA \"pINF\" ID " + indexStr + " \"scripts/bin/" + infoFilename + "\"\n"
        rsrcStr += "DATA \"pDSC\" ID " + indexStr + " \"scripts/bin/" + descFilename + "\"\n"

        baseSpritePath = "/home/tavisco/Downloads/pokeemerald-expansion-master/graphics/pokemon/"+pkmn.name+"/"
        spritePath = baseSpritePath + "front.png"
        palletePath = baseSpritePath + "normal.pal"

        sprite_file = Path(spritePath)
        if sprite_file.is_file():
            rsrcImgStr += "BITMAP ID " + indexStr + "\n"
            rsrcImgStr += "RSCTYPE \"pSPT\"\n"
            rsrcImgStr += "BEGIN\n"
            rsrcImgStr += "BITMAP \"scripts/img/pkmn/" + indexStr + "-8.bmp\" BPP 8\n"
            rsrcImgStr += "END\n"

            spriteBmpPath = "BMP3:img/pkmn/"+indexStr+"-8.bmp"

            palleteFile = open(palletePath)
            # read the content of the file opened
            content = palleteFile.readlines()

            transpColor = content[3]
            transpStr = transpColor.replace('\n', '').replace(" ", ",").split(",")
            hexColor = rgb_to_hex(transpStr[0], transpStr[1], transpStr[2])

            cmd = ["convert", spritePath, "-transparent", hexColor, "-background", "white", "-alpha", "remove",
            "-depth", "8", "-type", "palette", spriteBmpPath]
            fconvert = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            stdout, stderr = fconvert.communicate()
            assert fconvert.returncode == 0, stderr

    print("Building type chart binay data...")
    with open("pkmn_type_chart.json") as file:
        typDataset = json.load(file)
        for count, typeMain in enumerate(PkmnTypes, start=1):
            typeBytes = bytearray()
            for countSpec, typeSub in enumerate(PkmnTypes, start=1):
                effectiveness = typDataset[PkmnTypes[count]][PkmnTypes[countSpec]]
                # print(PkmnTypes[count], end='')
                # print(' x ', end='')
                # print(PkmnTypes[countSpec], end='')
                # print(': ', end='')
                # print(effectiveness, end='')
                # print("\n")
                typeBytes += struct.pack("B", effectiveness)

            indexStr = str(count).rjust(4, '0')
            effectivenessFilename = "pEFF" + indexStr + ".bin"
            with open("bin/" + effectivenessFilename, "wb") as file:
                file.write(typeBytes)
            rsrcStr += "DATA \"pEFF\" ID " + indexStr + " \"scripts/bin/" + effectivenessFilename + "\"\n"

    print("Building resource file...")
    with open("to_resource.txt", "wb") as file:
            file.write(bytearray(rsrcStr, "ascii"))

    print("Building images resource file...")
    with open("to_img_resource.txt", "wb") as file:
            file.write(bytearray(rsrcImgStr, "ascii"))

    print("Building names file...")
    with open("names.txt", "wb") as file:
            file.write(bytearray(pkmnNames, "ascii"))

    print("All done!")

