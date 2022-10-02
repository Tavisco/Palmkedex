import pokebase as pb

def word_to_hex(word):
    enc = word.encode("ascii")
    return enc.hex()

def get_type(pkmnType) -> int:
    match pkmnType:
        case "normal":
            return 1
        case "fighting":
            return 2
        case "flying":
            return 3
        case "poison":
            return 4
        case "ground":
            return 5
        case "rock":
            return 6
        case "bug":
            return 7
        case "ghost":
            return 8
        case "steel":
            return 9
        case "fire":
            return 10
        case "water":
            return 11
        case "grass":
            return 12
        case "electric":
            return 13
        case "psychic":
            return 14
        case "ice":
            return 15
        case "dragon":
            return 16
        case "dark":
            return 17
        case "fairy":
            return 18
        case "unknown":
            return 19
        case "shadow":
            return 20
        case "none":
            return 21
        
if __name__=="__main__":
    # 905 pokemons
    pkmnQuantity = 905

    rsrcStr = ""
    pkmnNames = "#define PKMN_QUANTITY = " + str(pkmnQuantity) + "\n"
    
    for i in range(1, pkmnQuantity+1):
        pkmn =  pb.pokemon(i)
        print(str(i) + " - " + pkmn.name)
        
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
        filename = "pINF" + indexStr + ".bin"

        with open("bin/" + filename, "wb") as file:
            file.write(b)
        
        rsrcStr += "DATA \"pINF\" ID " + indexStr + " \"scripts/bin/" + filename + "\"\n"
        pkmnNames += "{\"" + pkmn.name.ljust(11, ' ').capitalize() + "\"},\n"
    
    print("Building resource file")
    with open("to_resource.txt", "wb") as file:
            file.write(bytearray(rsrcStr, "ascii"))

    print("Building names file")
    with open("names.txt", "wb") as file:
            file.write(bytearray(pkmnNames, "ascii"))

    print("All done!")
