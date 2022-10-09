from ctypes import sizeof
import pokebase as pb
import unicodedata
import json
import struct

PkmnTypes = {1: 'normal', 2: 'fire', 3: 'water', 4: 'grass', 5: 'electric', 6: 'rock', 7: 'ground', 8: 'ice', 9: 'flying', 10: 'fighting', 11: 'ghost',
                12: 'bug', 13: 'poison', 14: 'psychic', 15: 'steel', 16: 'dark', 17: 'dragon', 18: 'fairy', 19: 'unknown', 20: 'shadow', 21: 'none'}

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
    pkmnQuantity = 5

    rsrcStr = ""
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

        pkmnNames += "{\"" + pkmn.name.ljust(11, ' ').capitalize() + "\"},\n"

        # s1 = pb.SpriteResource('pokemon', i)
        # spritePath = "img/"+indexStr+".png"
        # spriteBmpPath = "BMP3:img/"+indexStr+".bmp"
        # with open(spritePath, "wb") as file:
        #     file.write(s1.img_data)
        # # -filter point -interpolate Integer
        # cmd = ["convert", spritePath, "-background", "white", "-alpha", "remove", "-resize", "64x64", "-filter", "point", "-interpolate", "Integer", "-depth", "8", spriteBmpPath]
        # fconvert = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        # stdout, stderr = fconvert.communicate()
        # assert fconvert.returncode == 0, stderr

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
                typeBytes += struct.pack(">B", effectiveness)

            indexStr = str(count).rjust(4, '0')
            effectivenessFilename = "pEFF" + indexStr + ".bin"
            with open("bin/" + effectivenessFilename, "wb") as file:
                file.write(typeBytes)
            rsrcStr += "DATA \"pEFF\" ID " + indexStr + " \"scripts/bin/" + effectivenessFilename + "\"\n"

    print("Building resource file...")
    with open("to_resource.txt", "wb") as file:
            file.write(bytearray(rsrcStr, "ascii"))

    print("Building names file...")
    with open("names.txt", "wb") as file:
            file.write(bytearray(pkmnNames, "ascii"))

    print("All done!")

