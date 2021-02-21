package main

import (
	"fmt"
	"os"
	"encoding/json"
)

type Config struct {
	CbsdEnv		string	`json:"cbsdenv"`
	Broker		string	`json:"broker"`
	ImageList	string	`json:"imagelist"`
	BeanstalkConfig		`json:"beanstalkd"`
}

func LoadConfiguration(file string) (Config,error) {
	var config Config
	configFile, err := os.Open(file)
	defer configFile.Close()

	if err != nil {
		fmt.Println(err.Error())
		return config, err
	}

	jsonParser := json.NewDecoder(configFile)
	err = jsonParser.Decode(&config)

	if err != nil {
		fmt.Printf("config error: %s: %s\n", file,err.Error())
		return config, err
	}

	fmt.Printf("Using config file: %s\n", file)
	return config, err
}
