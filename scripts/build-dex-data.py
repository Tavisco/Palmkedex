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

    mon.name = soup.find('h1').text#.ljust(11, ' ').capitalize()

    #print('Current: ' + name)

    nextPkmnLink = soup.find('a', class_='entity-nav-next')
    nextPkmn = ""
    if (nextPkmnLink):
        nextPkmn = nextPkmnLink['href']
    mon.next_url = nextPkmn


    vitalTable = soup.find_all("table", {"class": "vitals-table"})[0].find_all("tr")

    mon.num = vitalTable[0].find("td").text.strip()
    mon.icon_url = 'https://archives.bulbagarden.net/wiki/File:' + mon.num + 'MS8.png'

    mon.num = mon.num.rjust(4, '0')

    types = vitalTable[1].find("td").text.strip().split(" ")
    mon.type1 = get_type(types[0])
    mon.type2 = get_type(types[1]) if len(types) > 1 else 21    # Type 2

    statsTable = soup.find_all("table", {"class": "vitals-table"})[3].find_all("tr")

    mon.hp = statsTable[0].find("td").text.strip()
    mon.attack = statsTable[1].find("td").text.strip()
    mon.defense = statsTable[2].find("td").text.strip()
    mon.sp_attack = statsTable[3].find("td").text.strip()
    mon.sp_defense = statsTable[4].find("td").text.strip()
    mon.speed = statsTable[5].find("td").text.strip()

    descTable = soup.find_all("td", {"class": "cell-med-text"})[0]
    mon.description = descTable.text.strip()

    mon.hres_url = soup.find("main").find_all("img")[0]['src']
    lres = soup.find_all("img", {"class": "img-sprite-v1"})
    if (lres):
        mon.lres_url = lres[0]['src']
        print('lres')
    

    

    print('#'+mon.num + ' ' + mon.name + ' - T1: ' + str(mon.type1) + ' - T2: ' + str(mon.type2) + ' - ' + mon.hres_url + ' - ' + mon.description)
    return mon

#print(soup.select("#tab-basic-1 > div:nth-child(1) > div.grid-col.span-md-6.span-lg-4.text-center > p:nth-child(1) > a > picture > img")[0]['src']) # HIGH RES

if __name__=="__main__":
    mons = []
    nextMon = "/pokedex/treecko"

    print("start")
    while (nextMon):
        currentMon = get_mon(nextMon)
        mons.append(currentMon)
        nextMon = currentMon.next_url

        if len(mons) == 10:
            break

    print("ok")

#