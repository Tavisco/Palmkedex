import pokebase as pb
import unicodedata
import json
import struct
import subprocess
from pathlib import Path

# UPDATE THOSE CONSTANTS TO MATCH YOUR ENVIRONMENT!
# You need ImageMagick installed on you system, specially the 'convert' tool

# Get it from https://github.com/rh-hideout/pokeemerald-expansion/tree/master
pokeEmeralExpansionPath = "/home/tavisco/Downloads/pokeemerald-expansion-master"

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

def getPkmnDescription(entries):
    for entry in entries:
        if entry.language.name == "en":
            desc = entry.flavor_text.replace(u'\f',       u'\n') \
                          .replace(u'\u00ad\n', u'') \
                          .replace(u'\u00ad',   u'') \
                          .replace(u' -\n',     u' - ') \
                          .replace(u'-\n',      u'-') \
                          .replace(u'\n',       u' ')
            desc = strip_accents(desc)
            desc = desc.replace("POKeMON", "POKEMON")
            desc +=  "\0"
            return desc

def generateSpriteForBpp(pkmnName, indexStr, depthStr):
    baseSpritePath = pokeEmeralExpansionPath + "/graphics/pokemon/"+pkmnName+"/"
    spritePath = baseSpritePath + "front.png"
    # The pallete file is necessary to remove the background color
    # from the image
    palletePath = baseSpritePath + "normal.pal"

    sprite_file = Path(spritePath)

    rsrcImgStr = ""
    # Firstly, we check if the file exists, if not
    # we just ignore this pokemon
    if sprite_file.is_file():

        # Then we prepare the string for this image
        # that will be copied to the resources file
        rsrcImgStr +=  "DATA \"pSPT\" ID " + indexStr + " \"scripts/img/pkmn/" + indexStr + ".png\"\n"

        spriteBmpPath = "img/pkmn/"+indexStr+".png"

        palleteFile = open(palletePath)
        # read the content of the pallete file
        content = palleteFile.readlines()

        # The background color, is on the 4th line
        # thus the 3rd item of the array
        transpColor = content[3]
        # And then we format it to be in the RGB Triplet format
        transpStr = transpColor.replace('\n', '').replace(" ", ",").split(",")

        # Then, convert it to HEX, that ImageMagick expects
        hexColor = rgb_to_hex(transpStr[0], transpStr[1], transpStr[2])

        # Prepare the command

        # convert /home/tavisco/Downloads/pokeemerald-expansion-master/graphics/pokemon/treecko/front.png -transparent '#00b0e8' -background white -alpha remove img/pkmn/252.png
        # convert front.png -transparent '#00b0e8' -background white -alpha remove 252.png
        cmd = ["convert", spritePath, "-transparent", hexColor,
                 "-background", "white", "-alpha", "remove",
                 spriteBmpPath
                 ]

        # And execute it
        fconvert = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = fconvert.communicate()

        assert fconvert.returncode == 0, stderr

        # pngcrush -ow -fix -force -nofilecheck -brute -rem alla -oldtimestamp 252.png

        # Now to crush the PNG
        cmd = ["pngcrush", "-ow-", "-fix", "-force",
                 "-nofilecheck", "-brute", "-rem", "alla",
                 "-oldtimestamp", spriteBmpPath
                 ]

        # And execute it
        fconvert = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = fconvert.communicate()

        assert fconvert.returncode == 0, stderr
    else:
        print("***** COULD NOT FIND SPRITE! *****")

    return rsrcImgStr

if __name__=="__main__":
    print("Welcome! This script will prepare the pokedex data for Palmkedex.")
    # As of writing this script, there are 905 pokemons
    pkmnQuantity = 905

    print("We will get data for " + str(pkmnQuantity) + " pokemons.")

    rsrcStr = ""
    rsrcImgStr8bpp = ""
    rsrcImgStr2bpp = ""
    pkmnNames = "" 
    
    for i in range(493, pkmnQuantity+1):
        # Get data from the API
        pkmn =  pb.pokemon(i)
        spc = pb.pokemon_species(i)

        pkmnDesc = getPkmnDescription(spc.flavor_text_entries)
        print("#" + str(i) + " - " + pkmn.name + ": " + pkmnDesc)

        # Add data to name's array, that will generate the C header.
        pkmnNames += "{\"" + pkmn.name.ljust(11, ' ').capitalize() + "\"},\n"

        # One byte per stat.
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

        # Makes 25 --> 0025, and use that as the number
        # on the resources
        indexStr = str(i).rjust(4, '0')
        infoFilename = "pINF" + indexStr + ".bin"
        descFilename = "pDSC" + indexStr + ".bin"

        # Write the stats bytes
        with open("bin/" + infoFilename, "wb") as file:
            file.write(b)

        # Write the description as bytes
        with open("bin/" + descFilename, "wb") as file:
            file.write(pkmnDesc.encode('utf-8'))
        
        # Add the files we just generated onto the file that
        # will be copied to the resource file
        rsrcStr += "DATA \"pINF\" ID " + indexStr + " \"scripts/bin/" + infoFilename + "\"\n"
        rsrcStr += "DATA \"pDSC\" ID " + indexStr + " \"scripts/bin/" + descFilename + "\"\n"

        # Now we start processing the sprites from the GBA source
        # rsrcImgStr8bpp += generateSpriteForBpp(pkmn.name, indexStr, "2")

    print("All pokemons were fetched!")

    print("Building type chart binay data...")
    with open("pkmn_type_chart.json") as file:
        # Load the type char JSON
        typDataset = json.load(file)
        for count, typeMain in enumerate(PkmnTypes, start=1):
            # We create an empty bytearray
            typeBytes = bytearray()
            for countSpec, typeSub in enumerate(PkmnTypes, start=1):
                effectiveness = int(typDataset[PkmnTypes[count]][PkmnTypes[countSpec]] * 100)
                # And start adding the data as binary. One byte per type.
                typeBytes += struct.pack("B", effectiveness)

            # After all bytes are added, we must save the
            # data as binary
            indexStr = str(count).rjust(4, '0')
            effectivenessFilename = "pEFF" + indexStr + ".bin"
            with open("bin/" + effectivenessFilename, "wb") as file:
                file.write(typeBytes)

            # And then, add the corresponding entry for the resource file
            rsrcStr += "DATA \"pEFF\" ID " + indexStr + " \"scripts/bin/" + effectivenessFilename + "\"\n"
        print("Done!")

    print("Building resource file...")
    with open("to_resource.txt", "wb") as file:
            file.write(bytearray(rsrcStr, "ascii"))
    print("Done!")

    # print("Building images resource file...")
    # with open("to_img_resource-8bpp.txt", "wb") as file:
    #         file.write(bytearray(rsrcImgStr8bpp, "ascii"))
    # print("Done!")

    print("Building names file...")
    with open("names.txt", "wb") as file:
            file.write(bytearray(pkmnNames, "ascii"))
    print("Done!")

    print("All done!")
    print("Don't forget to add the resources generated to their actual files!")
    print("Correct the pokemon names lentgh too, if they are above 11 chars, it will need to be trimmed for the code to compile.")
    print("If the pokemon count has changed, update this line as below on Palmkedex.h:")
    print("#define PKMN_QUANTITY = " + str(pkmnQuantity))
