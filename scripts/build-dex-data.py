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

PkmnTypes = {1: 'Normal', 2: 'Fire', 3: 'Water', 4: 'Grass', 5: 'Electric', 6: 'Rock', 7: 'Ground', 8: 'Ice', 9: 'Flying', 10: 'Fighting', 11: 'Ghost',
                12: 'Bug', 13: 'Poison', 14: 'Psychic', 15: 'Steel', 16: 'Dark', 17: 'Dragon', 18: 'Fairy', 19: 'Unknown', 20: 'Shadow', 21: 'None'}

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
    mon.type1 = get_type(types[0])
    mon.type2 = get_type(types[1]) if len(types) > 1 else 21    # Type 2

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

    print('#'+monNumStr + ' ' + mon.name + ' scrapped.')
    return mon

if __name__=="__main__":
    mons = List[Pokemon]
    nextMon = "/pokedex/bulbasaur"

    print("Welcome! This script will prepare the pokedex data for Palmkedex.")
    print("Scraping all pokemon data...")
    while (nextMon):
        currentMon = get_mon(nextMon)
        mons.append(currentMon)
        nextMon = currentMon.next_url

        if len(mons) == 10:
            break
    
    print("Pokemon data successfully scrapped!")
    print("Fetching low-res pokemon sprites...")
    for mon in mons:
        print(mon.name)
    print("Low-res pokemon sprites successfully fetched!")
    print("Fetching high-res pokemon sprites...")

    print("High-res pokemon sprites successfully fetched!")
    print("Fetching pokemon icon sprites...")

    print("Pokemon icon sprites successfully fetched!")
    print("Building pINF bin and resource files") # Pokemon's info

    print("pINF bin and resource files successfully built!")
    print("Building pDSC bin and resource files") # Pokemon's dex entry

    print("pDSC bin and resource files successfully built!")
    print("Building pNME bin and resource files") # Pokemon;s name

    print("pNME bin and resource files successfully built!")
    print("Building pEFF bin and resource files") # Global efficiency table

    print("pEFF bin and resource files successfully built!")
    print("Building pHSP bin and resource files") # High-Res sprites

    print("pHSP bin and resource files successfully built!")
    print("Building pLSP bin and resource files") # Low-Res sprites

    print("pLSP bin and resource files successfully built!")
    print("Building pICO bin and resource files") # Icon sprites

    print("pICO bin and resource files successfully built!")
    print("Done! Everything has been built.")