package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"unicode"

	"github.com/PuerkitoBio/goquery"
	"golang.org/x/text/runes"
	"golang.org/x/text/transform"
	"golang.org/x/text/unicode/norm"
)

type Pokemon struct {
	name         string
	num          int
	formattedNum string
	type1        int
	type2        int
	hp           int
	attack       int
	defense      int
	spAttack     int
	spDefense    int
	speed        int
	description  string
	hresUrl      string
	lresUrl      string
	iconUrl      string
	nextMon      string
}

const (
	iconURL               = "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/versions/generation-vii/icons/%d.png"
	hresURL               = "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/other/official-artwork/%d.png"
	lresURL               = "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/%d.png"
	binFolder             = "bin"
	descriptionFolder     = "description"
	descriptionTxtFile1   = "description1.txt"
	descriptionTxtFile2   = "description2.txt"
	descriptionBinFile1   = "description1.bin"
	descriptionBinFile2   = "description2.bin"
	descriptionCountSplit = 906
)

var PkmnTypes = []string{
	"normal",
	"fire",
	"water",
	"grass",
	"electric",
	"rock",
	"ground",
	"ice",
	"flying",
	"fighting",
	"ghost",
	"bug",
	"poison",
	"psychic",
	"steel",
	"dark",
	"dragon",
	"fairy",
	"unknown",
	"shadow",
	"none",
}

var resourceFiles = []string{
	"sprites_hres_16bpp.rcp",
	"sprites_hres_4bpp.rcp",
	"sprites_mres_16bpp.rcp",
	"sprites_mres_4bpp.rcp",
	"sprites_mres_2bpp.rcp",
	"sprites_mres_1bpp.rcp",
	"sprites_lres_16bpp.rcp",
	"sprites_lres_4bpp.rcp",
	"sprites_lres_2bpp.rcp",
	"sprites_lres_1bpp.rcp",
}

// give a pokemon description, strips all accents and special characters
func removeAllAccents(description string) string {
	// Remove all accents
	t := transform.Chain(norm.NFD, runes.Remove(runes.In(unicode.Mn)), norm.NFC)
	result, _, _ := transform.String(t, description)

	// Remove all special characters, but comma and dot
	reg := regexp.MustCompile("[^a-zA-Z0-9,. ]+")
	result = reg.ReplaceAllString(result, "")

	// Make POKEMON always uppercase
	reg = regexp.MustCompile(`(?i)pokemon`)
	result = reg.ReplaceAllString(result, "POKEMON")

	return result
}

func getType(pkmnType string) int {
	for index, item := range PkmnTypes {
		if item == strings.ToLower(pkmnType) {
			return index + 1
		}
	}
	return 21
}

// parses String to int and remove non-numeric characters and clean the string before parsing
func parseIntClean(value string) int {
	i, err := strconv.Atoi(strings.ReplaceAll(strings.ReplaceAll(value, "\n", ""), " ", ""))
	if err != nil {
		log.Fatal(err)
	}
	return i
}

// given a pokemon name, fetch its data from https://pokemondb.net/pokedex/<name>
func fetchPokemonData(pokemonName string) (Pokemon, error) {
	var pokemon Pokemon

	url := fmt.Sprintf("https://pokemondb.net/pokedex/%s", pokemonName)

	resp, err := http.Get(url)
	if err != nil {
		log.Fatal("Failed to fetch the page:", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		log.Fatalf("\nstatus code error: %d %s", resp.StatusCode, resp.Status)
	}

	doc, err := goquery.NewDocumentFromReader(resp.Body)
	if err != nil {
		log.Fatal("Failed to parse the HTML:", err)
	}

	// Create a new pokemon
	pokemon.name = removeAllAccents(doc.Find("h1").Text())

	nextUrl := doc.Find(".entity-nav-next").AttrOr("href", "")
	pokemon.nextMon = strings.ReplaceAll(nextUrl, "/pokedex/", "")

	pkmnNum, err := strconv.Atoi(doc.Find(".vitals-table").Find("tbody").Find("tr").Eq(0).Find("td").Eq(0).Text())
	if err != nil {
		log.Fatalf("\nFailed to parse pokemon number: %e", err)
	}
	pokemon.num = pkmnNum

	// Images
	pokemon.iconUrl = fmt.Sprintf(iconURL, pokemon.num)
	pokemon.hresUrl = fmt.Sprintf(hresURL, pokemon.num)
	pokemon.lresUrl = fmt.Sprintf(lresURL, pokemon.num)

	// Types
	pokemon.type1 = getType(doc.Find(".vitals-table").Find("tbody").Find("tr").Eq(1).Find("td").Eq(0).Find("a").Eq(0).Text())
	pokemon.type2 = getType(doc.Find(".vitals-table").Find("tbody").Find("tr").Eq(1).Find("td").Eq(0).Find("a").Eq(1).Text())

	// Stats
	statsTable := doc.Find("table.vitals-table").Eq(3).Find("tr")

	pokemon.hp = parseIntClean(statsTable.Eq(0).Find("td").Eq(0).Text())
	pokemon.attack = parseIntClean(statsTable.Eq(1).Find("td").Eq(0).Text())
	pokemon.defense = parseIntClean(statsTable.Eq(2).Find("td").Eq(0).Text())
	pokemon.spAttack = parseIntClean(statsTable.Eq(3).Find("td").Eq(0).Text())
	pokemon.spDefense = parseIntClean(statsTable.Eq(4).Find("td").Eq(0).Text())
	pokemon.speed = parseIntClean(statsTable.Eq(5).Find("td").Eq(0).Text())
	// Description
	pokemon.description = removeAllAccents(doc.Find("td.cell-med-text").Eq(0).Text())

	// Format the pokemon number
	pokemon.formattedNum = fmt.Sprintf("%04d", pokemon.num)

	// Print pokemon successfully scraped with its number first
	fmt.Printf("#%s %-*s scrapped.", pokemon.formattedNum, 13, pokemon.name)

	return pokemon, nil
}

func deleteDirectoryIfExist(dir string) {
	if _, err := os.Stat(dir); err == nil {
		os.RemoveAll(dir)
	}
}

// download a file from url and save it to dest
func downloadFile(url string, dest string) error {
	// Send HTTP GET request to the URL
	resp, err := http.Get(url)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		log.Fatalf("\nstatus code error: %d %s", resp.StatusCode, resp.Status)
	}

	currDir, err := os.Getwd()
	if err != nil {
		log.Fatalf("\nFailed to get current directory: %e", err)
	}

	dest = currDir + dest

	// If the file already exists, bail out
	if _, err := os.Stat(dest); err == nil {
		return nil
	}

	// If the directory does not exist, create it
	if _, err := os.Stat(filepath.Dir(dest)); os.IsNotExist(err) {
		os.MkdirAll(filepath.Dir(dest), os.ModePerm)
	}

	// Create the destination file
	file, err := os.Create(dest)
	if err != nil {
		log.Fatalf("\nFailed to get create destination file: %e", err)
		return err
	}
	defer file.Close()

	// Copy the response body to the destination file
	n, err := io.Copy(file, resp.Body)
	if err != nil {
		log.Fatalf("\nFailed to copy body: %e", err)
	}

	// Check if the file size matches the Content-Length header
	if resp.ContentLength > 0 && n != resp.ContentLength {
		log.Fatalf("\nFailed to download file: %e", err)
	}

	return nil
}

func compressWithACI(fmtNum string, sourceFolder string, outputFolder string, bpp int) {
	cwd, _ := os.Getwd()
	outputPathFolder := filepath.Join(cwd, outputFolder)
	os.MkdirAll(outputPathFolder, os.ModePerm)

	// If the output file already exists, bail out
	outputPath := filepath.Join(outputPathFolder, fmtNum+".bin")
	if _, err := os.Stat(outputPath); err == nil {
		return
	}

	// If the source file does not exist, bail out
	sourceSpritePath := filepath.Join(cwd, sourceFolder, fmtNum+".png")
	if _, err := os.Stat(sourceSpritePath); os.IsNotExist(err) {
		return
	}

	// Common non-16bpp arguments
	cmdArgs := []string{
		"-dither", "FloydSteinberg", "-colorspace", "gray",
	}

	switch bpp {
	case 1:
		cmdArgs = append(cmdArgs, "-monochrome")
	case 2:
		cmdArgs = append(cmdArgs, "-colors", "4", "-normalize")
	case 4:
		cmdArgs = append(cmdArgs, "-colors", "16")
	case 16:
		cmdArgs = []string{
			"+dither", "-colors", "25",
		}
	default:
		log.Fatalf("\nInvalid bpp: %d", bpp)
	}
	cmdArgs = append(cmdArgs, "-type", "truecolor", "tmp.bmp")

	cmd := exec.Command("convert", append([]string{sourceSpritePath}, cmdArgs...)...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	if err := cmd.Run(); err != nil {
		log.Fatalf("\nError executing convert command: %v\n", err)
	}

	aciCmd := ""
	if bpp != 16 {
		aciCmd = fmt.Sprintf("../aci/aci c%d < tmp.bmp > %s", bpp, outputPath)
	} else {
		aciCmd = fmt.Sprintf("../aci/aci c < tmp.bmp > %s", outputPath)
	}

	if err := exec.Command("sh", "-c", aciCmd).Run(); err != nil {
		log.Fatalf("\nError executing ACI compression command: %v\n", err)
	}
}

func removePngBackground(file string) {
	currDir, err := os.Getwd()
	if err != nil {
		log.Fatalf("\nFailed to get current directory: %e", err)
		return
	}

	file = currDir + file

	// If the file does not exist, bail out
	if _, err := os.Stat(file); os.IsNotExist(err) {
		return
	}

	// Remove the background
	cmd := exec.Command("convert", file, "-background", "white", "-alpha", "remove", file)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		log.Fatalf("\nError executing convert command: %v\n", err)
	}
}

// given a png image path and its output path, resize the image in a 1:1 ratio
func resizePngImage(input string, output string, size int) {
	currDir, err := os.Getwd()
	if err != nil {
		log.Fatalf("\nFailed to get current directory: %e", err)
	}

	input = currDir + input
	output = currDir + output

	// If the file does not exist, bail out
	if _, err := os.Stat(input); os.IsNotExist(err) {
		return
	}

	if _, err := os.Stat(filepath.Dir(output)); os.IsNotExist(err) {
		os.MkdirAll(filepath.Dir(output), os.ModePerm)
	}

	// Resize the image
	cmd := exec.Command("convert", input, "-resize", fmt.Sprintf("%dx%d", size, size), output)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		log.Fatalf("\nError executing convert command: %v\n", err)
	}
}

func compressDescriptionListWithDescrcompress(inputFile string, outputFile string) {
	cwd, _ := os.Getwd()

	basepath := filepath.Join(cwd, binFolder, descriptionFolder)
	sourcePath := filepath.Join(basepath, inputFile)
	outputPath := filepath.Join(basepath, outputFile)

	// If the source file does not exist, bail out
	if _, err := os.Stat(sourcePath); os.IsNotExist(err) {
		return
	}

	// If the output file already exists, delete it
	if _, err := os.Stat(outputPath); err == nil {
		os.Remove(outputPath)
	}

	// Compress the description list
	cmdStr := fmt.Sprintf("../descrcompress/compress c < %s > %s", sourcePath, outputPath)
	cmd := exec.Command("sh", "-c", cmdStr)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		log.Fatalf("\nError executing descrcompress command: %v\n", err)
	}
}

func appendToResourceFiles(fmtNum string) {
	cwd, _ := os.Getwd()

	basepath := filepath.Join(cwd, "to-resources")

	if _, err := os.Stat(basepath); os.IsNotExist(err) {
		os.MkdirAll(basepath, os.ModePerm)
	}

	for _, fileName := range resourceFiles {
		outputTxtPath := filepath.Join(basepath, fileName)
		file, err := os.OpenFile(outputTxtPath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
		if err != nil {
			log.Fatalf("\nError opening file: %v\n", err)
		}
		defer file.Close()

		resourceName := strings.ReplaceAll(fileName[:len(fileName)-len(".rcp")], "_", "/")
		resourcePath := filepath.Join("bin", resourceName, fmtNum+".bin")
		file.WriteString(fmt.Sprintf("DATA \"pSPT\" ID %s \"tools/dexdata/%s\"\n", fmtNum, resourcePath))
	}
}

func appendToDescriptionFile(desc string, monNum int) {
	cwd, _ := os.Getwd()

	basepath := filepath.Join(cwd, binFolder, descriptionFolder)

	if _, err := os.Stat(basepath); os.IsNotExist(err) {
		os.MkdirAll(basepath, os.ModePerm)
	}

	outputTxtPath := ""
	if monNum < descriptionCountSplit {
		outputTxtPath = filepath.Join(basepath, descriptionTxtFile1)
	} else {
		outputTxtPath = filepath.Join(basepath, descriptionTxtFile2)
	}
	file, err := os.OpenFile(outputTxtPath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		log.Fatalf("\nError opening file: %v\n", err)
	}
	defer file.Close()

	file.WriteString(fmt.Sprintf("%s\n", desc))
}

// given a pokemon, generate a binary file for its status and type
func generateInfoBinFile(pokemon Pokemon) {
	basepath := "../infoMake/data"
	if _, err := os.Stat(basepath); os.IsNotExist(err) {
		os.MkdirAll(basepath, os.ModePerm)
	}

	outputPath := filepath.Join(basepath, pokemon.formattedNum+".bin")

	// If the output file already exists, bail out
	if _, err := os.Stat(outputPath); err == nil {
		return
	}

	// Create the output file
	file, err := os.OpenFile(outputPath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		log.Fatalf("\nError opening file: %v\n", err)
	}
	defer file.Close()

	info := []byte{
		byte(pokemon.hp),
		byte(pokemon.attack),
		byte(pokemon.defense),
		byte(pokemon.spAttack),
		byte(pokemon.spDefense),
		byte(pokemon.speed),
		byte(pokemon.type1),
		byte(pokemon.type2),
	}

	file.Write(info)
}

func appendNameToTemplateFile(name string) {

	basepath := "../infoMake/data"

	outputTxtPath := filepath.Join(basepath, "mon-names.txt")

	file, err := os.OpenFile(outputTxtPath, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		log.Fatalf("\nError opening file: %v\n", err)
	}
	defer file.Close()

	file.WriteString(fmt.Sprintf("{\"%s\"},\n", name))
}

func main() {
	fmt.Println("Welcome! This script will prepare the pokedex data for Palmkedex.")
	fmt.Println("Cleaning up old data...")
	deleteDirectoryIfExist("to-resources/")
	deleteDirectoryIfExist("bin/")
	deleteDirectoryIfExist("../infoMake/data/")

	monName := "Bulbasaur"

	fmt.Println("Fetching data...")
	for {
		pokemon, err := fetchPokemonData(monName)
		if err != nil {
			log.Fatalf("\nFailed to fetch pokemon data: %e", err)
		}

		// Download sprites
		// downloadFile(pokemon.icon_url, fmt.Sprintf("/downloads/icon/%s.png", pokemon.formatted_num))
		// removePngBackground(fmt.Sprintf("/downloads/icon/%s.png", pokemon.formatted_num))
		// fmt.Print("[X]ICON ")

		downloadFile(pokemon.lresUrl, fmt.Sprintf("/downloads/lres/%s.png", pokemon.formattedNum))
		removePngBackground(fmt.Sprintf("/downloads/lres/%s.png", pokemon.formattedNum))
		compressWithACI(pokemon.formattedNum, "/downloads/lres", "/bin/sprites/lres/1bpp", 1)
		compressWithACI(pokemon.formattedNum, "/downloads/lres", "/bin/sprites/lres/2bpp", 2)
		compressWithACI(pokemon.formattedNum, "/downloads/lres", "/bin/sprites/lres/4bpp", 4)
		compressWithACI(pokemon.formattedNum, "/downloads/lres", "/bin/sprites/lres/16bpp", 16)
		fmt.Print("[X]LRES ")

		downloadFile(pokemon.hresUrl, fmt.Sprintf("/downloads/hres/%s.png", pokemon.formattedNum))
		removePngBackground(fmt.Sprintf("/downloads/hres/%s.png", pokemon.formattedNum))
		resizePngImage(fmt.Sprintf("/downloads/hres/%s.png", pokemon.formattedNum), fmt.Sprintf("/downloads/hres/%s.png", pokemon.formattedNum), 192)
		compressWithACI(pokemon.formattedNum, "/downloads/hres", "/bin/sprites/hres/4bpp", 4)
		compressWithACI(pokemon.formattedNum, "/downloads/hres", "/bin/sprites/hres/16bpp", 16)
		fmt.Print("[X]HRES ")

		resizePngImage(fmt.Sprintf("/downloads/hres/%s.png", pokemon.formattedNum), fmt.Sprintf("/downloads/mres/%s.png", pokemon.formattedNum), 144)
		compressWithACI(pokemon.formattedNum, "/downloads/mres", "/bin/sprites/mres/1bpp", 1)
		compressWithACI(pokemon.formattedNum, "/downloads/mres", "/bin/sprites/mres/2bpp", 2)
		compressWithACI(pokemon.formattedNum, "/downloads/mres", "/bin/sprites/mres/4bpp", 4)
		compressWithACI(pokemon.formattedNum, "/downloads/mres", "/bin/sprites/mres/16bpp", 16)
		fmt.Print("[X]MRES ")

		appendToResourceFiles(pokemon.formattedNum)
		appendToDescriptionFile(pokemon.description, pokemon.num)
		fmt.Print("[X]RESOURCES ")

		generateInfoBinFile(pokemon)
		appendNameToTemplateFile(pokemon.name)
		fmt.Print("[X]INFO ")

		fmt.Print("\n")
		monName = pokemon.nextMon

		if monName == "" {
			break
		}
	}

	fmt.Println("Compressing description list...")
	compressDescriptionListWithDescrcompress(descriptionTxtFile1, descriptionBinFile1)
	compressDescriptionListWithDescrcompress(descriptionTxtFile2, descriptionBinFile2)

	fmt.Println("Done! All data was successfully prepared.")
}
