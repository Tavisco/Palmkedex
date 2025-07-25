#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char itemsNames[][23] = {
{"Ability Capsule"},
{"Ability Patch"},
{"Ability Shield"},
{"Ability Urge"},
{"Abomasite"},
{"Absolite"},
{"Absorb Bulb"},
{"Academy Ball"},
{"Academy Bottle"},
{"Academy Cup"},
{"Academy Tablecloth"},
{"Adamant Crystal"},
{"Adamant Mint"},
{"Adamant Orb"},
{"Adrenaline Orb"},
{"Aerodactylite"},
{"Aggronite"},
{"Aguav Berry"},
{"Air Balloon"},
{"Alakazite"},
{"Alomomola Mucus"},
{"Aloraichium Z"},
{"Altarianite"},
{"Ampharosite"},
{"Amulet Coin"},
{"Antidote"},
{"Apicot Berry"},
{"Apple"},
{"Applin Juice"},
{"Apricorn"},
{"Armor Fossil"},
{"Armorite Ore"},
{"Arrokuda Scales"},
{"Aspear Berry"},
{"Assault Vest"},
{"Audinite"},
{"Auspicious Armor"},
{"Aux Evasion"},
{"Aux Guard"},
{"Aux Power"},
{"Aux Powerguard"},
{"Avocado"},
{"Awakening"},
{"Axew Scales"},
{"Azurill Fur"},
{"Babiri Berry"},
{"Bach's Food Tin"},
{"Bacon"},
{"Bagon Scales"},
{"Baguette"},
{"Ball of Mud"},
{"Balm Mushroom"},
{"Banana"},
{"Banettite"},
{"Barboach Slime"},
{"Barred Cup"},
{"Basculin Fang"},
{"Basil"},
{"Battle Tablecloth"},
{"Beach Glass"},
{"Bean Cake"},
{"Beast Ball"},
{"Beedrillite"},
{"Belue Berry"},
{"Bergmite Ice"},
{"Berry Juice"},
{"Berry Sweet"},
{"Big Bamboo Shoot"},
{"Big Malasada"},
{"Big Mushroom"},
{"Big Nugget"},
{"Big Pearl"},
{"Big Root"},
{"Binding Band"},
{"Bitter Herba Mystica"},
{"Black Apricorn"},
{"Black Augurite"},
{"Black Belt"},
{"Black Flute"},
{"Black Glasses"},
{"Black Sludge"},
{"Black Tumblestone"},
{"Blank Plate"},
{"Blastoisinite"},
{"Blazikenite"},
{"Blue Apricorn"},
{"Blue Bottle"},
{"Blue Cup"},
{"Blue Dish"},
{"BlueFlag Pick"},
{"Blue Flute"},
{"Blue Poke Ball Pick"},
{"Blue Scarf"},
{"Blue Shard"},
{"BlueSky Flower Pick"},
{"Blue Tablecloth"},
{"Bluk Berry"},
{"Blunder Policy"},
{"Bob's Food Tin"},
{"Boiled Egg"},
{"Bold Mint"},
{"Bombirdier Feather"},
{"Bonsly Tears"},
{"Booster Energy"},
{"Bottle Cap"},
{"Bounsweet Sweat"},
{"Bramblin Twig"},
{"Brave Mint"},
{"Bread"},
{"Bright Powder"},
{"Brittle Bones"},
{"Bronze Bottle"},
{"Bronze Cup"},
{"Bronzor Fragment"},
{"Bruxish Tooth"},
{"Bug Gem"},
{"Bug Memory"},
{"Bug Tera Shard"},
{"Buginium Z"},
{"Bugwort"},
{"Buizel Fur"},
{"Burn Drive"},
{"Burn Heal"},
{"Butter"},
{"BW Grass Tablecloth"},
{"Cacnea Needle"},
{"CakeLure Base"},
{"Calcium"},
{"Calm Mint"},
{"Cameruptite"},
{"Candy Truffle"},
{"Capsakid Seed"},
{"Carbos"},
{"Careful Mint"},
{"Carrot Seeds"},
{"Casteliacone"},
{"Caster Fern"},
{"Cell Battery"},
{"Cetoddle Grease"},
{"Chalky Stone"},
{"Charcadet Soot"},
{"Charcoal"},
{"Charizardite X"},
{"Charizardite Y"},
{"Charti Berry"},
{"Cheese"},
{"Cheri Berry"},
{"Cherish Ball"},
{"Cherry Tomatoes"},
{"Chesto Berry"},
{"Chewtle Claw"},
{"Chilan Berry"},
{"Chili Sauce"},
{"Chill Drive"},
{"Chipped Pot"},
{"Choice Band"},
{"Choice Dumpling"},
{"Choice Scarf"},
{"Choice Specs"},
{"Chople Berry"},
{"Chorizo"},
{"Clauncher Claw"},
{"Claw Fossil"},
{"Cleanse Tag"},
{"Clear Amulet"},
{"Clever Feather"},
{"Clever Mochi"},
{"Clover Sweet"},
{"Coba Berry"},
{"Coconut Milk"},
{"Colbur Berry"},
{"Combee Honey"},
{"Comet Shard"},
{"Cornn Berry"},
{"Courage Candy"},
{"Courage Candy L"},
{"Courage Candy XL"},
{"Cover Fossil"},
{"Covert Cloak"},
{"Crabrawler Shell"},
{"Cracked Pot"},
{"Crafting Kit"},
{"Cream Cheese"},
{"Croagunk Poison"},
{"Crunchy Salt"},
{"Cryogonal Ice"},
{"Cubchoo Fur"},
{"Cucumber"},
{"Cufant Tarnish"},
{"Curry Powder"},
{"Custap Berry"},
{"Cyber Ball"},
{"Cyclizar Scales"},
{"Damp Mulch"},
{"Damp Rock"},
{"Dark Gem"},
{"Dark Memory"},
{"Dark Tera Shard"},
{"Darkinium Z"},
{"Dawn Stone"},
{"Dazzling Honey"},
{"Decidium Z"},
{"Dedenne Fur"},
{"Deep Sea Scale"},
{"Deep Sea Tooth"},
{"Deerling Hair"},
{"Deino Scales"},
{"Delibird Parcel"},
{"Destiny Knot"},
{"Diamond Bottle"},
{"Diamond Pattern Cup"},
{"Diamond Tablecloth"},
{"Diancite"},
{"Digger Drill"},
{"Diglett Dirt"},
{"Dire Hit"},
{"Dire Hit 2"},
{"Dire Hit 3"},
{"Direshroom"},
{"Discount Coupon"},
{"Discovery Slate"},
{"Distortion Slate"},
{"Ditto Goo"},
{"Dive Ball"},
{"DNA Splicers"},
{"Dome Fossil"},
{"Dondozo Whisker"},
{"Doppel Bonnets"},
{"Douse Drive"},
{"Draco Plate"},
{"Dragon Fang"},
{"Dragon Gem"},
{"Dragon Memory"},
{"Dragon Scale"},
{"Dragon Tera Shard"},
{"Dragonium Z"},
{"Dratini Scales"},
{"Dread Plate"},
{"Dream Ball"},
{"Dreepy Powder"},
{"Drifloon Gas"},
{"Dropped Item"},
{"Drowzee Fur"},
{"Dubious Disc"},
{"Dunsparce Scales"},
{"Durin Berry"},
{"Dusk Ball"},
{"Dusk Stone"},
{"Dynamax Candy"},
{"Dynite Ore"},
{"Earth Plate"},
{"Eevee Cup"},
{"Eevee Fur"},
{"Eevium Z"},
{"Egg"},
{"Eiscue Down"},
{"Eject Button"},
{"Eject Pack"},
{"Electirizer"},
{"Electric Gem"},
{"Electric Memory"},
{"Electric Seed"},
{"Electric Tera Shard"},
{"Electrium Z"},
{"Elevator Key"},
{"Elixir"},
{"Energy Powder"},
{"Energy Root"},
{"Enigma Berry"},
{"Escape Rope"},
{"Ether"},
{"Everstone"},
{"Eviolite"},
{"Exercise Ball"},
{"Exp. Candy L"},
{"Exp. Candy M"},
{"Exp. Candy S"},
{"Exp. Candy XL"},
{"Exp. Candy XS"},
{"Exp. Share"},
{"Expert Belt"},
{"Fairium Z"},
{"Fairy Gem"},
{"Fairy Memory"},
{"Fairy Tera Shard"},
{"Falinks Sweat"},
{"Fancy Apple"},
{"Fast Ball"},
{"Feather Ball"},
{"Festival Ticket"},
{"Fidough Fur"},
{"Fighting Gem"},
{"Fighting Memory"},
{"Fighting Tera Shard"},
{"Fightinium Z"},
{"Figy Berry"},
{"Fine Remedy"},
{"Finizen Mucus"},
{"Finneon Scales"},
{"Fire Gem"},
{"Fire Memory"},
{"Fire Pattern Cup"},
{"Fire Stone"},
{"Fire Tera Shard"},
{"Firium Z"},
{"Fist Plate"},
{"Flabebe Pollen"},
{"Flame Orb"},
{"Flame Plate"},
{"Flamigo Down"},
{"Fletchling Feather"},
{"Flittle Down"},
{"Float Stone"},
{"Flower Pattern Cup"},
{"Flower Sweet"},
{"Fluffy Tail"},
{"Flying Gem"},
{"Flying Memory"},
{"Flying Tera Shard"},
{"Flyinium Z"},
{"Focus Band"},
{"Focus Sash"},
{"Fomantis Leaf"},
{"Foongus Spores"},
{"Fossilized Bird"},
{"Fossilized Dino"},
{"Fossilized Drake"},
{"Fossilized Fish"},
{"Fresh Cream"},
{"FreshStart Mochi"},
{"Fresh Water"},
{"Fried Fillet"},
{"Fried Food"},
{"Friend Ball"},
{"Frigibax Scales"},
{"Fruit Bunch"},
{"Full Heal"},
{"Full Incense"},
{"Full Restore"},
{"Galarica Cuff"},
{"Galarica Twig"},
{"Galarica Wreath"},
{"Galladite"},
{"Ganlon Berry"},
{"Garchompite"},
{"Gardevoirite"},
{"Gastly Gas"},
{"Gengarite"},
{"Genius Feather"},
{"Genius Mochi"},
{"Genome Slate"},
{"Gentle Mint"},
{"Ghost Gem"},
{"Ghost Memory"},
{"Ghost Tera Shard"},
{"Ghostium Z"},
{"Gible Scales"},
{"Gigantamix"},
{"Gigaton Ball"},
{"Gimmighoul Coin"},
{"Girafarig Fur"},
{"Glalitite"},
{"Glimmet Crystal"},
{"Gold Bottle"},
{"Gold Bottle Cap"},
{"Gold Cup"},
{"Gold Leaf"},
{"Gold Pick"},
{"Golden Nanab Berry"},
{"Golden Pinap Berry"},
{"Golden Razz Berry"},
{"Gooey Mulch"},
{"Goomy Goo"},
{"Gothita Eyelash"},
{"Grain Cake"},
{"Grass Gem"},
{"Grass Memory"},
{"Grass Tera Shard"},
{"Grassium Z"},
{"Grassy Seed"},
{"Great Ball"},
{"Greavard Wax"},
{"Green Apricorn"},
{"Green Bell Pepper"},
{"Green Dish"},
{"Green Poke Ball Pick"},
{"Green Scarf"},
{"Green Shard"},
{"Grepa Berry"},
{"Grimer Toxin"},
{"Grip Claw"},
{"Griseous Core"},
{"Griseous Orb"},
{"Grit Dust"},
{"Grit Gravel"},
{"Grit Pebble"},
{"Grit Rock"},
{"Ground Gem"},
{"Ground Memory"},
{"Ground Tera Shard"},
{"Groundium Z"},
{"Growlithe Fur"},
{"Growth Mulch"},
{"Grubby Hanky"},
{"Guard Spec."},
{"Gulpin Mucus"},
{"Gyaradosite"},
{"Haban Berry"},
{"Ham"},
{"Hamburger"},
{"Happiny Dust"},
{"Hard Stone"},
{"Hasty Mint"},
{"Hatenna Dust"},
{"Hawlucha Down"},
{"Heal Ball"},
{"Heal Powder"},
{"Health Candy"},
{"Health Candy L"},
{"Health Candy XL"},
{"Health Feather"},
{"Health Mochi"},
{"Heart Scale"},
{"Hearty Grains"},
{"Heat Rock"},
{"Heavy Ball"},
{"HeavyDuty Boots"},
{"Helix Fossil"},
{"Heracronite"},
{"Heracross Claw"},
{"Herbed Sausage"},
{"Heroic Sword Pick"},
{"Hitech Earbuds"},
{"Hippopotas Sand"},
{"HM01"},
{"HM02"},
{"HM03"},
{"HM04"},
{"HM05"},
{"HM06"},
{"HM07"},
{"HM08"},
{"Hometown Muffin"},
{"Hondew Berry"},
{"Honey"},
{"Honey Cake"},
{"Honor Of Kalos"},
{"Hopo Berry"},
{"Hoppip Leaf"},
{"Horseradish"},
{"Houndoominite"},
{"Houndour Fang"},
{"HP Up"},
{"Hyper Potion"},
{"Iapapa Berry"},
{"Ice Gem"},
{"Ice Heal"},
{"Ice Memory"},
{"Ice Stone"},
{"Ice Tera Shard"},
{"Iceroot Carrot"},
{"Icicle Plate"},
{"Icium Z"},
{"Icy Rock"},
{"Igglybuff Fluff"},
{"Impidimp Hair"},
{"Impish Mint"},
{"Incinium Z"},
{"Indeedee Fur"},
{"Insect Plate"},
{"Instant Noodles"},
{"Intriguing Stone"},
{"Iron"},
{"Iron Ball"},
{"Iron Barktongue"},
{"Iron Chunk"},
{"Iron Plate"},
{"Item Drop"},
{"Item Urge"},
{"Jaboca Berry"},
{"Jalapeno"},
{"Jam"},
{"Jet Ball"},
{"Johto Slate"},
{"Jolly Mint"},
{"Jubilife Muffin"},
{"Kangaskhanite"},
{"Kanto Slate"},
{"Kasib Berry"},
{"Kebia Berry"},
{"Kee Berry"},
{"Kelpsy Berry"},
{"Ketchup"},
{"King's Leaf"},
{"King's Rock"},
{"Kiwi"},
{"Klawf Claw"},
{"Klawf Stick"},
{"Klefki Key"},
{"Kofu's Wallet"},
{"Komala Claw"},
{"Kommonium Z"},
{"Koraidon's Poke Ball"},
{"Kricketot Shell"},
{"Lagging Tail"},
{"Lansat Berry"},
{"Large Leek"},
{"Larvesta Fuzz"},
{"Larvitar Claw"},
{"Latiasite"},
{"Latiosite"},
{"Lava Cookie"},
{"Lax Incense"},
{"Lax Mint"},
{"Leaden Ball"},
{"Leader's Crest"},
{"Leaf Letter"},
{"Leaf Stone"},
{"Leafy Tablecloth"},
{"Lechonk Hair"},
{"Leek"},
{"Leftovers"},
{"Legend Plate"},
{"Lemonade"},
{"Leppa Berry"},
{"Lettuce"},
{"Level Ball"},
{"Liechi Berry"},
{"Life Orb"},
{"Light Ball"},
{"Light Clay"},
{"Lilac Tablecloth"},
{"Linking Cord"},
{"Litleo Tuft"},
{"Loaded Dice"},
{"Lone Earring"},
{"Lonely Mint"},
{"Looker Ticket"},
{"Lopunnite"},
{"Lost Satchel"},
{"Love Ball"},
{"Love Sweet"},
{"Lucarionite"},
{"Luck Incense"},
{"Lucky Egg"},
{"Lucky Punch"},
{"Lum Berry"},
{"Luminous Moss"},
{"Lumiose Galette"},
{"Lunalium Z"},
{"Lure"},
{"Lure Ball"},
{"Lustrous Globe"},
{"Lustrous Orb"},
{"Luvdisc Scales"},
{"Luxury Ball"},
{"Lycanium Z"},
{"Macho Brace"},
{"Magical Heart Pick"},
{"Magical Star Pick"},
{"Magikarp Scales"},
{"Magmarizer"},
{"Magnemite Screw"},
{"Magnet"},
{"Mago Berry"},
{"Magost Berry"},
{"Makuhita Sweat"},
{"Malicious Armor"},
{"Manectite"},
{"Mankey Fur"},
{"Maranga Berry"},
{"Marble"},
{"Mareanie Spike"},
{"Mareep Wool"},
{"Marill Ball"},
{"Marmalade"},
{"Marsh Balm"},
{"Marshadium Z"},
{"Maschiff Fang"},
{"Master Ball"},
{"Masterpiece Teacup"},
{"Mawilite"},
{"Max Elixir"},
{"Max Ether"},
{"Max Honey"},
{"Max Lure"},
{"Max Mushrooms"},
{"Max Potion"},
{"Max Repel"},
{"Max Revive"},
{"Mayonnaise"},
{"Meadow Plate"},
{"Medichamite"},
{"Medicinal Leek"},
{"Meditite Sweat"},
{"Mental Herb"},
{"Meowth Fur"},
{"Metagrossite"},
{"Metal Alloy"},
{"Metal Coat"},
{"Metal Powder"},
{"Metronome"},
{"Mewnium Z"},
{"Mewtwonite X"},
{"Mewtwonite Y"},
{"Micle Berry"},
{"Mighty Candy"},
{"Mighty Candy L"},
{"Mighty Candy XL"},
{"Mild Mint"},
{"Mimikium Z"},
{"Mimikyu Scrap"},
{"Mind Plate"},
{"Mint Tablecloth"},
{"Miracle Seed"},
{"Miraidon's Poke Ball"},
{"Mirror Herb"},
{"Misdreavus Tears"},
{"Misty Seed"},
{"Mixed Mushrooms"},
{"Modest Mint"},
{"Monstrous Tablecloth"},
{"Moomoo Cheese"},
{"Moomoo Milk"},
{"Moon Ball"},
{"Moon Stone"},
{"Mudbray Mud"},
{"Murkrow Bauble"},
{"Muscle Band"},
{"Muscle Feather"},
{"Muscle Mochi"},
{"Mushroom Cake"},
{"Mustard"},
{"Mysterious Shard L"},
{"Mysterious Shard S"},
{"Mystic Water"},
{"Nacli Salt"},
{"Naive Mint"},
{"Nanab Berry"},
{"Naughty Mint"},
{"Nest Ball"},
{"Net Ball"},
{"NeverMelt Ice"},
{"Noibat Fur"},
{"Nomel Berry"},
{"Noodles"},
{"Normal Gem"},
{"Normal Tera Shard"},
{"Normalium Z"},
{"Nugget"},
{"Numel Lava"},
{"Nymble Claw"},
{"Occa Berry"},
{"Oceanic Slate"},
{"Odd Incense"},
{"Odd Keystone"},
{"Old Amber"},
{"Old Gateau"},
{"Olive Oil"},
{"Onion"},
{"Oran Berry"},
{"Orange Dish"},
{"Oranguru Fur"},
{"Oricorio Feather"},
{"Origin Ball"},
{"Origin Ore"},
{"Orthworm Tarnish"},
{"Oval Charm"},
{"Oval Stone"},
{"Pachirisu Fur"},
{"Pack of Potatoes"},
{"Packaged Curry"},
{"Pamtre Berry"},
{"Paralyze Heal"},
{"Parasol Pick"},
{"Park Ball"},
{"Party Sparkler Pick"},
{"Pass Orb"},
{"Passho Berry"},
{"Passimian Fur"},
{"Pasta"},
{"Pawmi Fur"},
{"Pawniard Blade"},
{"Payapa Berry"},
{"Peach Tablecloth"},
{"Peanut Butter"},
{"Pearl"},
{"Pearl String"},
{"Peat Block"},
{"Pecha Berry"},
{"PepUp Plant"},
{"Pepper"},
{"Permit"},
{"Persim Berry"},
{"Petaya Berry"},
{"Petilil Leaf"},
{"Pewter Crunchies"},
{"Phanpy Nail"},
{"Pichu Fur"},
{"Pickle"},
{"Picnic Set"},
{"Pidgeotite"},
{"PikaPika Pick"},
{"Pikachu Cup"},
{"Pikanium Z"},
{"Pikashunium Z"},
{"Pinap Berry"},
{"Pincurchin Spines"},
{"Pineapple"},
{"Pineco Husk"},
{"Pink Apricorn"},
{"Pink Bottle"},
{"Pink Cup"},
{"Pink Nectar"},
{"Pink Scarf"},
{"Pink Tablecloth"},
{"Pinsirite"},
{"Pixie Plate"},
{"Plaid Tablecloth B"},
{"Plaid Tablecloth R"},
{"Plaid Tablecloth Y"},
{"Plasma Card"},
{"Plume Fossil"},
{"Plump Beans"},
{"Poison Barb"},
{"Poison Gem"},
{"Poison Memory"},
{"Poison Tera Shard"},
{"Poisonium Z"},
{"Poke Ball"},
{"Poke Doll"},
{"Poke Toy"},
{"Polished Mud Ball"},
{"PolkaDot Bottle"},
{"PolkaDot Cup"},
{"PolkaDot Tablecloth"},
{"Pomeg Berry"},
{"Pop Pod"},
{"Potato Salad"},
{"Potato Tortilla"},
{"Potion"},
{"Power Anklet"},
{"Power Band"},
{"Power Belt"},
{"Power Bracer"},
{"Power Herb"},
{"Power Lens"},
{"Power Plant Pass"},
{"Power Weight"},
{"PP Max"},
{"PP Up"},
{"Precooked Burger"},
{"Premier Ball"},
{"Pretty Feather"},
{"Primarium Z"},
{"Prism Scale"},
{"Prison Bottle"},
{"Prof's Letter"},
{"Prosciutto"},
{"Protective Pads"},
{"Protector"},
{"Protein"},
{"Psychic Gem"},
{"Psychic Memory"},
{"Psychic Seed"},
{"Psychic Tera Shard"},
{"Psychium Z"},
{"Psyduck Down"},
{"Punching Glove"},
{"Pungent Root"},
{"Pure Incense"},
{"Purple Nectar"},
{"Qualot Berry"},
{"Quick Ball"},
{"Quick Candy"},
{"Quick Candy L"},
{"Quick Candy XL"},
{"Quick Claw"},
{"Quick Powder"},
{"Quiet Mint"},
{"Qwilfish Spines"},
{"Rabuta Berry"},
{"Rage Candy Bar"},
{"Rainbow Slate"},
{"Ralts Dust"},
{"Rare Bone"},
{"Rare Candy"},
{"Rash Mint"},
{"Rawst Berry"},
{"Razor Claw"},
{"Razor Fang"},
{"Razz Berry"},
{"Reaper Cloth"},
{"Red Apricorn"},
{"Red Bell Pepper"},
{"Red Card"},
{"Red Dish"},
{"RedFlag Pick"},
{"Red Flute"},
{"Red Nectar"},
{"Red Onion"},
{"Red Poke Ball Pick"},
{"Red Scarf"},
{"Red Shard"},
{"Relaxed Mint"},
{"Relic Band"},
{"Relic Copper"},
{"Relic Crown"},
{"Relic Gold"},
{"Relic Silver"},
{"Relic Statue"},
{"Relic Vase"},
{"Rellor Mud"},
{"Remedy"},
{"Repeat Ball"},
{"Repel"},
{"Reset Urge"},
{"Resist Feather"},
{"Resist Mochi"},
{"Reveal Glass"},
{"Revival Herb"},
{"Revive"},
{"Ribbon Sweet"},
{"Rice"},
{"Rindo Berry"},
{"Ring Target"},
{"Riolu Fur"},
{"Rock Gem"},
{"Rock Incense"},
{"Rock Memory"},
{"Rock Tera Shard"},
{"Rockium Z"},
{"Rockruff Rock"},
{"Rocky Helmet"},
{"Roller Skates"},
{"Rolycoly Coal"},
{"Rookidee Feather"},
{"Room Service"},
{"Root Fossil"},
{"Rose Incense"},
{"Roseli Berry"},
{"Roto Bargain"},
{"Roto Boost"},
{"Roto Catch"},
{"Roto Encounter"},
{"Roto Exp. Points"},
{"Roto Friendship"},
{"Roto Hatch"},
{"Roto HP Restore"},
{"Roto PP Restore"},
{"Roto Prize Money"},
{"Roto Stealth"},
{"Rotom Phone"},
{"Rotom Sparks"},
{"Rowap Berry"},
{"Rufflet Feather"},
{"Sablenite"},
{"Sableye Gem"},
{"Sachet"},
{"Sacred Ash"},
{"Safari Ball"},
{"Safety Goggles"},
{"Salac Berry"},
{"Salad Mix"},
{"Salamencite"},
{"Salandit Gas"},
{"Salt"},
{"Salt Cake"},
{"Salty Herba Mystica"},
{"Sand Radish"},
{"Sandile Claw"},
{"Sandwich"},
{"Sandygast Sand"},
{"Sassy Mint"},
{"Sausages"},
{"Scarlet Book"},
{"Scatter Bang"},
{"Scatterbug Powder"},
{"Sceptilite"},
{"Scizorite"},
{"Scope Lens"},
{"Scroll of Darkness"},
{"Scroll of Waters"},
{"Scyther Claw"},
{"Sea Incense"},
{"Seed of Mastery"},
{"Serious Mint"},
{"Seviper Fang"},
{"Shaderoot Carrot"},
{"Shalour Sable"},
{"Sharp Beak"},
{"Sharpedonite"},
{"Shed Shell"},
{"Shell Bell"},
{"Shellder Pearl"},
{"Shellos Mucus"},
{"Shinx Fang"},
{"Shiny Charm"},
{"Shiny Stone"},
{"Shoal Salt"},
{"Shoal Shell"},
{"Shock Drive"},
{"Shroodle Ink"},
{"Shroomish Spores"},
{"Shuca Berry"},
{"Shuppet Scrap"},
{"Silicobra Sand"},
{"Silk Scarf"},
{"Silver Bottle"},
{"Silver Cup"},
{"Silver Leaf"},
{"Silver Nanab Berry"},
{"Silver Pick"},
{"Silver Pinap Berry"},
{"Silver Powder"},
{"Silver Razz Berry"},
{"Sinistea Chip"},
{"Sitrus Berry"},
{"Skiddo Leaf"},
{"Skrelp Kelp"},
{"Skull Fossil"},
{"Skwovet Fur"},
{"Sky Plate"},
{"Sky Tumblestone"},
{"Slakoth Fur"},
{"Slowbronite"},
{"Slowpoke Claw"},
{"Slowpoke Cup"},
{"Small Bouquet"},
{"Smart Candy"},
{"Smart Candy L"},
{"Smart Candy XL"},
{"Smiling Vee Pick"},
{"Smoke Ball"},
{"Smoke Bomb"},
{"SmokePoke Tail"},
{"Smoked Fillet"},
{"Smoliv Oil"},
{"Smooth Rock"},
{"Sneasel Claw"},
{"Snom Thread"},
{"Snorlium Z"},
{"Snorunt Fur"},
{"Snover Berries"},
{"Snowball"},
{"Soda Pop"},
{"Soft Sand"},
{"Solganium Z"},
{"Sootfoot Root"},
{"Soothe Bell"},
{"Soul Dew"},
{"Soul Slate"},
{"Sour Herba Mystica"},
{"Spell Tag"},
{"Spelon Berry"},
{"Spice Mix"},
{"Spicy Herba Mystica"},
{"Spiritomb Fragment"},
{"Splash Plate"},
{"Spoiled Apricorn"},
{"Spoink Pearl"},
{"Spooky Plate"},
{"Spooky Tablecloth"},
{"Sport Ball"},
{"Springy Mushroom"},
{"Sprinklotad"},
{"Squall Slate"},
{"Squawkabilly Feather"},
{"Stable Mulch"},
{"Stantler Hair"},
{"Star Piece"},
{"Star Sweet"},
{"Stardust"},
{"Starf Berry"},
{"Starly Feather"},
{"Stealth Spray"},
{"Steel Bottle B"},
{"Steel Bottle R"},
{"Steel Bottle Y"},
{"Steel Gem"},
{"Steel Memory"},
{"Steel Tera Shard"},
{"Steelium Z"},
{"Steelixite"},
{"Sticky Barb"},
{"Sticky Glob"},
{"Stone Plate"},
{"Stonjourner Stone"},
{"Strange Ball"},
{"Strange Souvenir"},
{"Stratospheric Slate"},
{"Strawberry"},
{"Strawberry Sweet"},
{"Stretchy Spring"},
{"Striped Bottle"},
{"Striped Cup"},
{"Striped Tablecloth"},
{"Stunky Fur"},
{"Sun Stone"},
{"Sunkern Leaf"},
{"Sunrise Flower Pick"},
{"Sunset Flower Pick"},
{"Super Lure"},
{"Super Potion"},
{"Super Repel"},
{"Surskit Syrup"},
{"Swablu Fluff"},
{"Swampertite"},
{"Swap Snack"},
{"Sweet Apple"},
{"Sweet Heart"},
{"Sweet Herba Mystica"},
{"Swift Feather"},
{"Swift Mochi"},
{"Swordcap"},
{"Syrupy Apple"},
{"Tadbulb Mucus"},
{"Tamato Berry"},
{"Tandemaus Fur"},
{"Tanga Berry"},
{"Tapunium Z"},
{"Tarountula Thread"},
{"Tart Apple"},
{"Tatsugiri Scales"},
{"Tauros Hair"},
{"Tectonic Slate"},
{"Teddiursa Claw"},
{"Tera Orb"},
{"Terrain Extender"},
{"Thick Club"},
{"Throat Spray"},
{"Thunder Stone"},
{"Timer Ball"},
{"Timid Mint"},
{"Tin of Beans"},
{"Tinkatink Hair"},
{"Tiny Bamboo Shoot"},
{"Tiny Mushroom"},
{"TM00"},
{"TM001"},
{"TM002"},
{"TM003"},
{"TM004"},
{"TM005"},
{"TM006"},
{"TM007"},
{"TM008"},
{"TM009"},
{"TM010"},
{"TM011"},
{"TM012"},
{"TM013"},
{"TM014"},
{"TM015"},
{"TM016"},
{"TM017"},
{"TM018"},
{"TM019"},
{"TM020"},
{"TM021"},
{"TM022"},
{"TM023"},
{"TM024"},
{"TM025"},
{"TM026"},
{"TM027"},
{"TM028"},
{"TM029"},
{"TM030"},
{"TM031"},
{"TM032"},
{"TM033"},
{"TM034"},
{"TM035"},
{"TM036"},
{"TM037"},
{"TM038"},
{"TM039"},
{"TM040"},
{"TM041"},
{"TM042"},
{"TM043"},
{"TM044"},
{"TM045"},
{"TM046"},
{"TM047"},
{"TM048"},
{"TM049"},
{"TM050"},
{"TM051"},
{"TM052"},
{"TM053"},
{"TM054"},
{"TM055"},
{"TM056"},
{"TM057"},
{"TM058"},
{"TM059"},
{"TM060"},
{"TM061"},
{"TM062"},
{"TM063"},
{"TM064"},
{"TM065"},
{"TM066"},
{"TM067"},
{"TM068"},
{"TM069"},
{"TM070"},
{"TM071"},
{"TM072"},
{"TM073"},
{"TM074"},
{"TM075"},
{"TM076"},
{"TM077"},
{"TM078"},
{"TM079"},
{"TM080"},
{"TM081"},
{"TM082"},
{"TM083"},
{"TM084"},
{"TM085"},
{"TM086"},
{"TM087"},
{"TM088"},
{"TM089"},
{"TM090"},
{"TM091"},
{"TM092"},
{"TM093"},
{"TM094"},
{"TM095"},
{"TM096"},
{"TM097"},
{"TM098"},
{"TM099"},
{"TM100"},
{"TM101"},
{"TM102"},
{"TM103"},
{"TM104"},
{"TM105"},
{"TM106"},
{"TM107"},
{"TM108"},
{"TM109"},
{"TM110"},
{"TM111"},
{"TM112"},
{"TM113"},
{"TM114"},
{"TM115"},
{"TM116"},
{"TM117"},
{"TM118"},
{"TM119"},
{"TM120"},
{"TM121"},
{"TM122"},
{"TM123"},
{"TM124"},
{"TM125"},
{"TM126"},
{"TM127"},
{"TM128"},
{"TM129"},
{"TM130"},
{"TM131"},
{"TM132"},
{"TM133"},
{"TM134"},
{"TM135"},
{"TM136"},
{"TM137"},
{"TM138"},
{"TM139"},
{"TM140"},
{"TM141"},
{"TM142"},
{"TM143"},
{"TM144"},
{"TM145"},
{"TM146"},
{"TM147"},
{"TM148"},
{"TM149"},
{"TM150"},
{"TM151"},
{"TM152"},
{"TM153"},
{"TM154"},
{"TM155"},
{"TM156"},
{"TM157"},
{"TM158"},
{"TM159"},
{"TM160"},
{"TM161"},
{"TM162"},
{"TM163"},
{"TM164"},
{"TM165"},
{"TM166"},
{"TM167"},
{"TM168"},
{"TM169"},
{"TM170"},
{"TM171"},
{"TM172"},
{"TM173"},
{"TM174"},
{"TM175"},
{"TM176"},
{"TM177"},
{"TM178"},
{"TM179"},
{"TM180"},
{"TM181"},
{"TM182"},
{"TM183"},
{"TM184"},
{"TM185"},
{"TM186"},
{"TM187"},
{"TM188"},
{"TM189"},
{"TM190"},
{"TM191"},
{"TM192"},
{"TM193"},
{"TM194"},
{"TM195"},
{"TM196"},
{"TM197"},
{"TM198"},
{"TM199"},
{"TM200"},
{"TM201"},
{"TM202"},
{"TM203"},
{"TM204"},
{"TM205"},
{"TM206"},
{"TM207"},
{"TM208"},
{"TM209"},
{"TM210"},
{"TM211"},
{"TM212"},
{"TM213"},
{"TM214"},
{"TM215"},
{"TM216"},
{"TM217"},
{"TM218"},
{"TM219"},
{"TM220"},
{"TM221"},
{"TM222"},
{"TM223"},
{"TM224"},
{"TM225"},
{"TM226"},
{"TM227"},
{"TM228"},
{"TM229"},
{"TMV Pass"},
{"Toedscool Flaps"},
{"Tofu"},
{"Tomato"},
{"Torkoal Coal"},
{"Tough Candy"},
{"Tough Candy L"},
{"Tough Candy XL"},
{"Toxel Sparks"},
{"Toxic Orb"},
{"Toxic Plate"},
{"TR00"},
{"TR01"},
{"TR02"},
{"TR03"},
{"TR04"},
{"TR05"},
{"TR06"},
{"TR07"},
{"TR08"},
{"TR09"},
{"TR10"},
{"TR11"},
{"TR12"},
{"TR13"},
{"TR14"},
{"TR15"},
{"TR16"},
{"TR17"},
{"TR18"},
{"TR19"},
{"TR20"},
{"TR21"},
{"TR22"},
{"TR23"},
{"TR24"},
{"TR25"},
{"TR26"},
{"TR27"},
{"TR28"},
{"TR29"},
{"TR30"},
{"TR31"},
{"TR32"},
{"TR33"},
{"TR34"},
{"TR35"},
{"TR36"},
{"TR37"},
{"TR38"},
{"TR39"},
{"TR40"},
{"TR41"},
{"TR42"},
{"TR43"},
{"TR44"},
{"TR45"},
{"TR46"},
{"TR47"},
{"TR48"},
{"TR49"},
{"TR50"},
{"TR51"},
{"TR52"},
{"TR53"},
{"TR54"},
{"TR55"},
{"TR56"},
{"TR57"},
{"TR58"},
{"TR59"},
{"TR60"},
{"TR61"},
{"TR62"},
{"TR63"},
{"TR64"},
{"TR65"},
{"TR66"},
{"TR67"},
{"TR68"},
{"TR69"},
{"TR70"},
{"TR71"},
{"TR72"},
{"TR73"},
{"TR74"},
{"TR75"},
{"TR76"},
{"TR77"},
{"TR78"},
{"TR79"},
{"TR80"},
{"TR81"},
{"TR82"},
{"TR83"},
{"TR84"},
{"TR85"},
{"TR86"},
{"TR87"},
{"TR88"},
{"TR89"},
{"TR90"},
{"TR91"},
{"TR92"},
{"TR93"},
{"TR94"},
{"TR95"},
{"TR96"},
{"TR97"},
{"TR98"},
{"TR99"},
{"Tropical Shell"},
{"Tropius Leaf"},
{"Tumblestone"},
{"TwiceSpiced Radish"},
{"Twisted Spoon"},
{"Tynamo Slime"},
{"Tyranitarite"},
{"Ultra Ball"},
{"Ultranecrozium Z"},
{"Unremarkable Teacup"},
{"Upgrade"},
{"Utility Umbrella"},
{"Varoom Fume"},
{"VeeVee Pick"},
{"Veluza Fillet"},
{"Venonat Fang"},
{"Venusaurite"},
{"Vinegar"},
{"Violet Book"},
{"Vivichoke"},
{"Voltorb Sparks"},
{"Wacan Berry"},
{"Wasabi"},
{"Water Gem"},
{"Water Memory"},
{"Water Stone"},
{"Water Tera Shard"},
{"Watercress"},
{"Waterium Z"},
{"Watmel Berry"},
{"Wattrel Feather"},
{"Wave Incense"},
{"Weakness Policy"},
{"Wepear Berry"},
{"Whimsical Tablecloth"},
{"Whipped Cream"},
{"Whipped Dream"},
{"White Apricorn"},
{"White Dish"},
{"White Flute"},
{"White Herb"},
{"Wide Lens"},
{"Wiglett Sand"},
{"Wiki Berry"},
{"Wing Ball"},
{"Wingull Feather"},
{"Winking Pika Pick"},
{"Wise Glasses"},
{"Wishing Piece"},
{"Wood"},
{"Wooper Slime"},
{"X Accuracy"},
{"X Accuracy 2"},
{"X Accuracy 3"},
{"X Accuracy 6"},
{"X Attack"},
{"X Attack 2"},
{"X Attack 3"},
{"X Attack 6"},
{"X Defense"},
{"X Defense 2"},
{"X Defense 3"},
{"X Defense 6"},
{"X Sp. Atk"},
{"X Sp. Atk 2"},
{"X Sp. Atk 3"},
{"X Sp. Atk 6"},
{"X Sp. Def"},
{"X Sp. Def 2"},
{"X Sp. Def 3"},
{"X Sp. Def 6"},
{"X Speed"},
{"X Speed 2"},
{"X Speed 3"},
{"X Speed 6"},
{"Yache Berry"},
{"Yarn Ball"},
{"Yellow Apricorn"},
{"Yellow Bell Pepper"},
{"Yellow Bottle"},
{"Yellow Cup"},
{"Yellow Dish"},
{"Yellow Flute"},
{"Yellow Nectar"},
{"Yellow Scarf"},
{"Yellow Shard"},
{"Yellow Tablecloth"},
{"Yogurt"},
{"Yungoos Fur"},
{"Zangoose Claw"},
{"Zap Plate"},
{"Zinc"},
{"Zoom Lens"},
{"Zorua Fur"},
};

static const char *types[] = {
	"battle items",
	"berries",
	"general items",
	"hold items",
	"machines",
	"medicine",
	"pokeballs",
	"unknown",
};

struct ItemInfo {	//must match provided resource data
	uint8_t type;
} __attribute__((packed));


static const char itemNameChars[] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-.'";

struct BB {
	uint8_t *dst;
	uint16_t buffer;
	uint8_t bitsUsed;
};

static void bbEmit(struct BB *bb, uint8_t val, uint8_t bits)
{
	bb->buffer += ((uint16_t)val) << bb->bitsUsed;
	bb->bitsUsed += bits;
	while (bb->bitsUsed >= 8) {
		*(bb->dst)++ = bb->buffer;
		bb->buffer >>= 8;
		bb->bitsUsed -= 8;
	}
}

static void bbFlush(struct BB *bb)
{
	while (bb->bitsUsed)
		bbEmit(bb, 1, 1);
}

int main(int argc, char** argv)
{
	unsigned i, ofst, numPokes = sizeof(itemsNames) / sizeof(*itemsNames), prevVal;
	uint8_t compressedData[numPokes][16];
	uint8_t compressedLengths[numPokes];
	struct ItemInfo infos[numPokes];

	for (i = 0; i < numPokes; i++) {
		char fname[32];
		FILE *f;
		
		sprintf(fname, "itemData/%04u.bin", i + 1);
		f = fopen(fname, "rb");
		if (f == NULL) {
			perror(fname);
			abort();
		}
		if (sizeof(struct ItemInfo) != fread(infos + i, 1, sizeof(struct ItemInfo), f))
			abort();
		fclose(f);
	}

	puts("//item info:");
	puts("// bytes of name (5 bit len, then 6 bits per char: 0-9A-Za-z-)");
	puts("// type (5 bits)");
	puts("HEX \"INFO\" ID 0");
	
	ofst = 2 * (1 + numPokes);
	
	//compress names and types
	for (i = 0; i < numPokes; i++) {
		
		struct BB bb = {.dst = compressedData[i], };
		unsigned j = 0;
		char name[23];
		
		strcpy(name, itemsNames[i]);
		
		while (strlen(name) < 4)	//max len is 11, so with a 3 bit len range we can encode 4..11, extend shorted ones with spaces. this is rare enough to make space savings worth it
			strcat(name, " ");
		
		bbEmit(&bb, strlen(name) - 4, 5);
		char ch;
		
		while ((ch = name[j++]) != 0) {
			
			char *at = strchr(itemNameChars, ch);
			unsigned encoded;
			
			if (!at) {
				
				fprintf(stderr, "Saw uncompressible char in poke name '%s'. Char was '%c'(%02xh)\n", itemsNames[i], ch, ch);
				abort();
			}
			bbEmit(&bb, at - itemNameChars, 6);
		}
		
		 //type
        bbEmit(&bb, infos[i].type, 5);

		bbFlush(&bb);
		compressedLengths[i] = bb.dst - compressedData[i];
	}
	printf("0x%02x 0x%02x //NUM item\n", (uint8_t)(numPokes >> 8), (uint8_t)numPokes);
	printf("//Now: offsets into this resource for each poke's data. Each poke occupies a minimum of 10 bytes (left as an exercise to you to prove this)\n");
	printf("//each offset is stores as 12 bits. A and B are stored as A.lo (A.hi + 16 * B.hi) B.lo\n");
	printf("//offsets are indexed from the FIRST full byte that follows ALL the offsets\n");
	printf("//offset for poke N is stored as (actual_offset - 8 * N)\n");
	
	ofst = 0;
	prevVal = 0;
	
	for (i = 0; i < numPokes; i++) {
		
		uint32_t effectiveOffset = ofst - 8 * i;
		
		if (effectiveOffset > 0x1000) {
			fprintf(stderr, "offset not encodeable\n");
			abort();
		}
		
		if (i & 1) {	//second value - emit

			printf(" 0x%02x 0x%02x 0x%02x // offsets for pokes %u and %u (%u and %u encoded)\n", 
					(uint8_t)prevVal, (uint8_t)(prevVal >> 8) + (uint8_t)((effectiveOffset >> 8) << 4), (uint8_t)effectiveOffset, i, i + 1, prevVal, effectiveOffset);
		}
		else {			//first value - record
			
			prevVal = effectiveOffset;
		}
		
		ofst += compressedLengths[i];
	}
	//emit last offset, if any, do not emit 3 bytes when two will do
	if (numPokes & 1) {
		printf(" 0x%02x 0x%02x // offset for pokes %u\n", 
					(uint8_t)prevVal, (uint8_t)(prevVal >> 8), i);
	}
	
	for (i = 0; i < numPokes; i++) {
		
		unsigned j;
		
		
		printf(	"\t//#%u (%s: %-8s):\n\t\t",
				i + 1, itemsNames[i], types[infos[i].type]);
		
		for (j = 0; j < compressedLengths[i]; j++) 
			printf("0x%02x ", compressedData[i][j]);
		printf(" //name and type\n");
	}
	
	puts("\n");
	puts("//item index");
	puts("//first: BE word offsets to start of each of the 26 chains");
	puts("//then: 26 chains.");
	puts("// each chain is a list of BE words, 0-terminated of item starting with that letter in increasing item order");

	
	printf("HEX \"INDX\" ID 0\n");
	uint16_t perLetter[26][1024], pos[26] = {};
	uint32_t totalOfst = 26;
	
	//list pokes by letter
	for (i = 0; i < sizeof(itemsNames) / sizeof(*itemsNames); i++) {
		
		uint16_t pokeId = i + 1;
		char firstLetter = itemsNames[i][0];
		
		if (firstLetter >= 'a' && firstLetter <= 'z')
			firstLetter += 'A' - 'a';
		
		perLetter[firstLetter - 'A'][pos[firstLetter - 'A']++] = pokeId;
	}
	
	//account for the terminators in lengths
	for (i = 0; i < 26; i++)
		perLetter[i][pos[i]++] = 0;
	
	//first the indices into the chain starts
	printf("\t//word offsets of the start of each chain:\n");
	for (i = 0; i < 26; i++) {
		printf(" 0x%02x 0x%02x\n", (uint8_t)(totalOfst >> 8), (uint8_t)totalOfst);
		totalOfst += pos[i];
	}
	printf("\n");
	//now the chains
	for (i = 0; i < 26; i++) {
		unsigned j;
		
		printf("\t//List of item for the letter '%c':\n\t\t", 'A' + i);
		for (j = 0; j < pos[i]; j++) {
			printf("  0x%02x 0x%02x", (uint8_t)(perLetter[i][j] >> 8), (uint8_t)perLetter[i][j]);
		}
		printf("\n");
	}
}