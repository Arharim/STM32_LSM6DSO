package main

import (
	"bufio"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

func convertLogToCSV(inputFile, outputFile string) error {
	in, err := os.Open(inputFile)
	if err != nil {
		return err
	}
	defer in.Close()

	out, err := os.Create(outputFile)
	if err != nil {
		return err
	}
	defer out.Close()

	scanner := bufio.NewScanner(in)
	headerFound := false
	index := 0

	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}

		if strings.HasPrefix(line, "gx_dps") {
			fmt.Fprintln(out, "Index,"+line)
			headerFound = true
			continue
		}

		if !headerFound {
			continue
		}

		fmt.Fprintf(out, "%d,%s\n", index, line)
		index++
	}

	if err := scanner.Err(); err != nil {
		return err
	}

	return nil
}

func main() {
	logsDir := filepath.Join("..", "logs")

	files, err := filepath.Glob(filepath.Join(logsDir, "*.log"))
	if err != nil {
		panic(err)
	}

	if len(files) == 0 {
		fmt.Println("No any .log files in the folder: ", logsDir)
		return
	}

	for _, inputFile := range files {
		baseName := strings.TrimSuffix(filepath.Base(inputFile), filepath.Ext(inputFile))
		outputFile := filepath.Join(logsDir, baseName+".csv")

		err := convertLogToCSV(inputFile, outputFile)
		if err != nil {
			fmt.Printf("Error during processing %s: %v\n", inputFile, err)
		} else {
			fmt.Printf("Ok %s → %s\n", filepath.Base(inputFile), filepath.Base(outputFile))
		}
	}

	fmt.Println("Conversion completed")
}
