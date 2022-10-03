import pokebase as pb
import unicodedata


def word_to_hex(word):
    enc = word.encode("ascii")
    return enc.hex()

def get_type(pkmnType) -> int:
    match pkmnType:
        case "normal":
            return 1
        case "fire":
            return 2
        case "water":
            return 3
        case "grass":
            return 4
        case "electric":
            return 5
        case "rock":
            return 6
        case "ground":
            return 7
        case "ice":
            return 8
        case "flying":
            return 9
        case "fighting":
            return 10
        case "ghost":
            return 11
        case "bug":
            return 12
        case "poison":
            return 13
        case "psychic":
            return 14
        case "steel":
            return 15
        case "dark":
            return 16
        case "dragon":
            return 17
        case "fairy":
            return 18
        case "unknown":
            return 19
        case "shadow":
            return 20
        case "none":
            return 21
        case "NoneType":
            return 21
        
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
    pkmnNames = "#define PKMN_QUANTITY = " + str(pkmnQuantity) + "\n"
    
    for i in range(1, pkmnQuantity+1):
        pkmn =  pb.pokemon(i)
        print(str(i) + " - " + pkmn.name)
        spc = pb.pokemon_species(i)
        flavor = getFlavor(spc.flavor_text_entries).replace(u'\f',       u'\n') \
                          .replace(u'\u00ad\n', u'') \
                          .replace(u'\u00ad',   u'') \
                          .replace(u' -\n',     u' - ') \
                          .replace(u'-\n',      u'-') \
                          .replace(u'\n',       u' ')
        flavor = strip_accents(flavor)
        flavor = flavor.replace("POKeMON", "POKEMON")
        flavor += "\0"
        print(flavor)
        
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
            file.write(flavor.encode('utf-8'))
        
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

    print("Building resource file")
    with open("to_resource.txt", "wb") as file:
            file.write(bytearray(rsrcStr, "ascii"))

    print("Building names file")
    with open("names.txt", "wb") as file:
            file.write(bytearray(pkmnNames, "ascii"))

    print("All done!")

