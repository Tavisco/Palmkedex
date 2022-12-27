import json
import os
import struct
import subprocess
from typing import List
import requests
from bs4 import BeautifulSoup
from dataclasses import dataclass

@dataclass
class Pokemon:
    name: str
    num: int
    formatted_num: int
    type1: int
    type2: int
    hp: int
    attack: int
    defense: int
    sp_attack: int
    sp_defense: int
    speed: int
    description: str
    hres_url: str
    lres_url: str
    grey_url: str
    icon_url: str
    next_url: str

PkmnTypes = {1: 'normal', 2: 'fire', 3: 'water', 4: 'grass', 5: 'electric', 6: 'rock', 7: 'ground', 8: 'ice', 9: 'flying', 10: 'fighting', 11: 'ghost',
                12: 'bug', 13: 'poison', 14: 'psychic', 15: 'steel', 16: 'dark', 17: 'dragon', 18: 'fairy', 19: 'unknown', 20: 'shadow', 21: 'none'}

def get_type(pkmnType) -> int:
    for count, item in enumerate(PkmnTypes, start=1):
        if PkmnTypes[item] == pkmnType:
            return count

def get_mon(uri) -> Pokemon:
    # Make a request to the webpage
    url = 'https://pokemondb.net'+uri
    response = requests.get(url)

    # Parse the HTML content
    soup = BeautifulSoup(response.text, 'html.parser')
    mon = Pokemon

    mon.name = soup.find('h1').text
    assert(mon.name)

    nextPkmnLink = soup.find('a', class_='entity-nav-next')
    nextPkmn = ""
    if (nextPkmnLink):
        nextPkmn = nextPkmnLink['href']
    mon.next_url = nextPkmn

    vitalTable = soup.find_all("table", {"class": "vitals-table"})[0].find_all("tr")

    assert("National №" == vitalTable[0].find("th").text.strip())
    mon.num = int(vitalTable[0].find("td").text.strip())
    monNumStr = str(mon.num)

    mon.icon_url = 'https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/versions/generation-vii/icons/' + monNumStr + '.png'
    assert(mon.icon_url)
    mon.hres_url = 'https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/other/official-artwork/' + monNumStr + '.png'
    assert(mon.hres_url)
    mon.lres_url = 'https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/' + monNumStr + '.png'
    assert(mon.lres_url)
    if (mon.num <= 151):
        mon.grey_url = 'https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/versions/generation-i/yellow/gray/' + monNumStr + '.png'

    assert(mon.num)

    assert("Type" == vitalTable[1].find("th").text.strip())
    types = vitalTable[1].find("td").text.strip().split(" ")
    mon.type1 = get_type(types[0].lower())
    mon.type2 = get_type(types[1].lower()) if len(types) > 1 else 21    # Type 2

    assert(mon.type1)
    assert(mon.type2)

    statsTable = soup.find_all("table", {"class": "vitals-table"})[3].find_all("tr")

    assert("HP" == statsTable[0].find("th").text.strip())
    mon.hp = int(statsTable[0].find("td").text.strip())
    assert("Attack" == statsTable[1].find("th").text.strip())
    mon.attack = int(statsTable[1].find("td").text.strip())
    assert("Defense" == statsTable[2].find("th").text.strip())
    mon.defense = int(statsTable[2].find("td").text.strip())
    assert("Sp. Atk" == statsTable[3].find("th").text.strip())
    mon.sp_attack = int(statsTable[3].find("td").text.strip())
    assert("Sp. Def" == statsTable[4].find("th").text.strip())
    mon.sp_defense = int(statsTable[4].find("td").text.strip())
    assert("Speed" == statsTable[5].find("th").text.strip())
    mon.speed = int(statsTable[5].find("td").text.strip())

    assert(mon.hp)
    assert(mon.attack)
    assert(mon.defense)
    assert(mon.sp_attack)
    assert(mon.sp_defense)
    assert(mon.speed)

    descTable = soup.find_all("td", {"class": "cell-med-text"})[0]
    mon.description = descTable.text.strip()
    assert(mon.description)

    mon.formatted_num = str(mon.num).rjust(4, '0')

    print('#'+mon.formatted_num + ' ' + mon.name + ' scrapped.', end=" ", flush=True)
    return mon

def download_convert_crush_png(mon, url, path, resize=False, resize_len="128"):
    # Send an HTTP GET request to the URL
    response = requests.get(url)
    
    cwd = os.getcwd()

    # Construct the path to the output folder
    output_path = os.path.join(cwd, path)

    # Create the output folder
    os.makedirs(output_path, exist_ok=True)
    
    spritePath = path+str(mon.num)+".png"

    # Open a file for writing in binary mode
    with open(spritePath, "wb") as f:
        # Write the content of the response to the file
        f.write(response.content)

    cmd = []
    if (resize):
        cmd = ["convert", spritePath,
                    "-background", "white", "-alpha", "remove",
                    "-resize", resize_len, 
                    spritePath+"_"
                    ]
    else:
        cmd = ["convert", spritePath,
            "-background", "white", "-alpha", "remove",
            spritePath+"_"
            ]

    # And execute it
    fconvert = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = fconvert.communicate()

    assert fconvert.returncode == 0, stderr

    cmd = ["convert", spritePath+"_",
                "-colors", "255", "-type", "palette", "-depth", "8",
                spritePath
                ]

    # And execute it
    fconvert = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = fconvert.communicate()

    assert fconvert.returncode == 0, stderr

    os.remove(spritePath+"_")

    # Now to crush the PNG
    cmd = ["pngcrush", "-ow-", "-fix", "-force",
                "-nofilecheck", "-brute", "-rem", "alla",
                "-oldtimestamp", spritePath
                ]

    # And execute it
    fconvert = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = fconvert.communicate()

    assert fconvert.returncode == 0, stderr

def build_efficiency_binaries():
    cwd = os.getcwd()

    # Construct the path to the output folder
    output_path = cwd + "/bin"
    rsrcStr = ""

    # Create the output folder
    os.makedirs(output_path, exist_ok=True)

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
            effectivenessFilename = "pEFF{}.bin".format(indexStr)
            with open(output_path + "/" + effectivenessFilename, "wb") as file:
                file.write(typeBytes)

            # And then, add the corresponding entry for the resource file
            rsrcStr += "DATA \"pEFF\" ID {} \"scripts/bin/{}\"\n".format(indexStr, effectivenessFilename)

    print("Writing pEFF resources file...")
    output_txt_path = cwd + "/to-resources"
    os.makedirs(output_txt_path, exist_ok=True)
    with open(output_txt_path + "/pEFF_resources.txt", "wb") as file:
            file.write(bytearray(rsrcStr, "ascii"))

def build_name_binary(mon):
    name = '#' + mon.formatted_num + ' ' + mon.name + "\0"
    monNameFilename = "bin/pNME{}.bin".format(mon.formatted_num)
    with open(monNameFilename, "wb") as file:
        file.write(name.encode('utf-8'))

def build_desc_binary(mon):
    desc = mon.description + "\0"
    desc = desc.replace("POKeMON", "POKEMON")
    desc = desc.replace("POKéMON", "POKEMON")
    desc = desc.replace("POKÉMON", "POKEMON")

    monNameFilename = "bin/pDSC{}.bin".format(mon.formatted_num)
    with open(monNameFilename, "wb") as file:
        file.write(desc.encode('utf-8'))

def build_inf_binary(mon):
    b = bytes([
            mon.hp,
            mon.attack,
            mon.defense,
            mon.sp_attack,
            mon.sp_defense,
            mon.speed,
            mon.type1,
            mon.type2
        ])

    infoFilename = "bin/pINF{}.bin".format(mon.formatted_num)
    with open(infoFilename, "wb") as file:
        file.write(b)

def build_resource_entries(mon):
    cwd = os.getcwd()
    output_txt_path = cwd + "/to-resources/mon_resources.txt"

    with open(output_txt_path, "a") as file:
        file.write("DATA \"pNME\" ID {} \"scripts/bin/pING{}.bin\"\n".format(mon.formatted_num, mon.formatted_num))
        file.write("DATA \"pDSC\" ID {} \"scripts/bin/pDSC{}.bin\"\n".format(mon.formatted_num, mon.formatted_num))
        file.write("DATA \"pINF\" ID {} \"scripts/bin/pINF{}.bin\"\n".format(mon.formatted_num, mon.formatted_num))

if __name__=="__main__":
    nextMon = "/pokedex/meltan"

    print("Welcome! This script will prepare the pokedex data for Palmkedex.")

    print("Building pEFF bin and resource files") # Global efficiency table
    build_efficiency_binaries()
    print("Done!")

    cwd = os.getcwd()
    output_txt_path = cwd + "/to-resources/mon_resources.txt"

    if os.path.exists(output_txt_path):
        os.remove(output_txt_path)

    print("Scraping all pokemon data...")
    while (nextMon):
        currentMon = get_mon(nextMon)
        
        download_convert_crush_png(currentMon, currentMon.hres_url, "img/hres/", resize=True)
        print("[X] HRES", end=" ", flush=True)

        download_convert_crush_png(currentMon, currentMon.lres_url, "img/lres/", resize=True, resize_len="64")
        print("[X] LRES", end=" ", flush=True)

        download_convert_crush_png(currentMon, currentMon.icon_url, "img/icon/")
        print("[X] ICON", end=" ", flush=True)
        
        if (currentMon.num <= 151):
            download_convert_crush_png(currentMon, currentMon.grey_url, "img/grey/", resize=True, resize_len="64")
            print("[X] GREY", end=" ", flush=True)
        else:
            print("[ ] GREY", end=" ", flush=True)

        build_name_binary(currentMon)
        print("[X] pNME", end=" ", flush=True) 

        build_desc_binary(currentMon)
        print("[X] pDSC", end=" ", flush=True)

        build_inf_binary(currentMon)
        print("[X] pINF", end=" ", flush=True)

        build_resource_entries(currentMon)
        print("[X] Resource entries", end=" ", flush=True)

        print("")
        nextMon = currentMon.next_url
    
    print("Pokemon data successfully scrapped!")

    print("Done! Everything has been built.")