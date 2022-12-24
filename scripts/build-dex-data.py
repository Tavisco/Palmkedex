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
    mon.hp = statsTable[0].find("td").text.strip()
    assert("Attack" == statsTable[1].find("th").text.strip())
    mon.attack = statsTable[1].find("td").text.strip()
    assert("Defense" == statsTable[2].find("th").text.strip())
    mon.defense = statsTable[2].find("td").text.strip()
    assert("Sp. Atk" == statsTable[3].find("th").text.strip())
    mon.sp_attack = statsTable[3].find("td").text.strip()
    assert("Sp. Def" == statsTable[4].find("th").text.strip())
    mon.sp_defense = statsTable[4].find("td").text.strip()
    assert("Speed" == statsTable[5].find("th").text.strip())
    mon.speed = statsTable[5].find("td").text.strip()

    assert(mon.hp)
    assert(mon.attack)
    assert(mon.defense)
    assert(mon.sp_attack)
    assert(mon.sp_defense)
    assert(mon.speed)

    descTable = soup.find_all("td", {"class": "cell-med-text"})[0]
    mon.description = descTable.text.strip()
    assert(mon.description)

    print('#'+monNumStr + ' ' + mon.name + ' scrapped.', end=" ", flush=True)
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
            effectivenessFilename = "pEFF" + indexStr + ".bin"
            with open(output_path + "/" + effectivenessFilename, "wb") as file:
                file.write(typeBytes)

            # And then, add the corresponding entry for the resource file
            rsrcStr += "DATA \"pEFF\" ID " + indexStr + " \"scripts/bin/" + effectivenessFilename + "\"\n"

    print("Writing pEFF resources file...")
    output_txt_path = cwd + "/to-resources"
    os.makedirs(output_txt_path, exist_ok=True)
    with open(output_txt_path + "/pEFF_resources.txt", "wb") as file:
            file.write(bytearray(rsrcStr, "ascii"))

if __name__=="__main__":
    count = 0
    nextMon = "/pokedex/bulbasaur"


    print("Welcome! This script will prepare the pokedex data for Palmkedex.")

    print("Building pEFF bin and resource files") # Global efficiency table
    build_efficiency_binaries()
    print("Done!")

    print("Scraping all pokemon data...")
    while (nextMon):
        if count == 9:
            break
        currentMon = get_mon(nextMon)
        
        download_convert_crush_png(currentMon, currentMon.hres_url, "img/hres/", resize=True)
        print("[X] HRES", end=" ", flush=True)
        download_convert_crush_png(currentMon, currentMon.lres_url, "img/lres/", resize=True, resize_len="64")
        print("[X] LRES", end=" ", flush=True)
        download_convert_crush_png(currentMon, currentMon.icon_url, "img/icon/")
        print("[X] ICON", end=" ", flush=True)
        download_convert_crush_png(currentMon, currentMon.grey_url, "img/grey/", resize=True, resize_len="64")

        print("[X] pNME", end=" ", flush=True)

        print("[X] pINF", end=" ", flush=True)
        
        print("")
        nextMon = currentMon.next_url
        count = count + 1
    
    print("Pokemon data successfully scrapped!")

    print("Fetching pokemon icon sprites...")

    print("Pokemon icon sprites successfully fetched!")
    print("Building pINF bin and resource files") # Pokemon's info

    print("pINF bin and resource files successfully built!")
    print("Building pDSC bin and resource files") # Pokemon's dex entry

    print("pDSC bin and resource files successfully built!")
    print("Building pNME bin and resource files") # Pokemon;s name

    print("pNME bin and resource files successfully built!")

    print("pEFF bin and resource files successfully built!")
    print("Building pHSP bin and resource files") # High-Res sprites

    print("pHSP bin and resource files successfully built!")
    print("Building pLSP bin and resource files") # Low-Res sprites

    print("pLSP bin and resource files successfully built!")
    print("Building pICO bin and resource files") # Icon sprites

    print("pICO bin and resource files successfully built!")
    print("Done! Everything has been built.")