// CBSD Project 2013-2021
// bhyve-bhyve project 2021
// Simple demo/sample for CBSD bhyve API
package main

import (
	"encoding/json"
	"github.com/gorilla/mux"
	"log"
	"net/http"
	"os"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"fmt"
	"reflect"
	"flag"
	"io/ioutil"
	"crypto/md5"
//	"github.com/getsentry/sentry-go"
)

var lock = sync.RWMutex{}
var config Config
var runscript string
var workdir string

type Response struct {
	Message    string
}

// The cluster Type. Name of elements must match with jconf params
type Vm struct {
	Jname		string  `json:jname,omitempty"`
	Img		string	`json:img,omitempty"`
	Ram		string	`json:ram,omitempty"`
	Cpus		string  `"cpus,omitempty"`
	Imgsize		string	`"imgsize,omitempty"`
	Pubkey		string	`"pubkey,omitempty"`
//	Email		string  `"email,omitempty"`
//	Callback	string  `"callback,omitempty"`
}
// Todo: validate mod?
//  e.g for simple check:
//  bhyve_name  string `json:"name" validate:"required,min=2,max=100"`

var (
	body		= flag.String("body", "", "Body of message")
	cbsdEnv		= flag.String("cbsdenv", "/usr/jails", "CBSD workdir environment")
	configFile	= flag.String("config", "/usr/local/etc/cbsd-mq-router.json", "Path to config.json")
	listen *string	= flag.String("listen", "0.0.0.0:65530", "Listen host:port")
	runScript	= flag.String("runscript", "bhyve-api", "CBSD target run script")
)

func fileExists(filename string) bool {
	info, err := os.Stat(filename)
	if os.IsNotExist(err) {
		return false
	}
	return !info.IsDir()
}

// main function to boot up everything
func main() {

	flag.Parse()
	var err error

//	serr := sentry.Init(sentry.ClientOptions{
//		Dsn: "https://<>",
//	})
//	if serr != nil {
//		log.Fatalf("sentry.Init: %s", serr)
//	}

	config, err = LoadConfiguration(*configFile)

//	sentry.CaptureException(err)

	runscript = *runScript
	workdir=config.CbsdEnv

	if err != nil {
		fmt.Println("config load error")
		os.Exit(1)
	}

	router := mux.NewRouter()
	router.HandleFunc("/api/v1/create/{instanceid}", HandleClusterCreate).Methods("POST")
	router.HandleFunc("/api/v1/status/{instanceid}", HandleClusterStatus).Methods("GET")
	router.HandleFunc("/api/v1/cluster", HandleClusterCluster).Methods("GET")
	router.HandleFunc("/api/v1/destroy/{instanceid}", HandleClusterDestroy).Methods("GET")
	fmt.Println("Listen",*listen)
	log.Fatal(http.ListenAndServe(*listen, router))
}

func HandleClusterStatus(w http.ResponseWriter, r *http.Request) {
	var instanceid string
	params := mux.Vars(r)
	instanceid = params["instanceid"]
	var regexpInstanceId = regexp.MustCompile(`^[aA-zZ_]([aA-zZ0-9_])*$`)

	Cid := r.Header.Get("cid")
	HomePath := fmt.Sprintf("/usr/home/%s/vms", Cid)
	//fmt.Println("CID IS: [ %s ]", cid)
	if _, err := os.Stat(HomePath); os.IsNotExist(err) {
		return
	}

	// check the name field is between 3 to 40 chars
	if len(instanceid) < 3 || len(instanceid) > 40 {
		http.Error(w, "The instance name must be between 3-40", 400)
		return
	}
	if !regexpInstanceId.MatchString(instanceid) {
		http.Error(w, "The instance name should be valid form, ^[aA-zZ_]([aA-zZ0-9_])*$", 400)
		return
	}
	SqliteDBPath := fmt.Sprintf("%s/jails-system/%s/bhyve.ssh", workdir,instanceid)
	if fileExists(SqliteDBPath) {
		b, err := ioutil.ReadFile(SqliteDBPath) // just pass the file name
		if err != nil {
			http.Error(w, "{}", 400)
			return
		} else {
			response := Response{string(b)}
			js, err := json.Marshal(response)
			if err != nil {
				http.Error(w, err.Error(), http.StatusInternalServerError)
				return
			}
			http.Error(w, string(js), 400)
			return
		}
	} else {
		http.Error(w, "{}", 400)
	}
}

func HandleClusterCluster(w http.ResponseWriter, r *http.Request) {
	Cid := r.Header.Get("cid")
	HomePath := fmt.Sprintf("/usr/home/%s/vms", Cid)
	//fmt.Println("CID IS: [ %s ]", cid)
	if _, err := os.Stat(HomePath); os.IsNotExist(err) {
		return
	}

	SqliteDBPath := fmt.Sprintf("/usr/home/%s/vm.list", Cid)
	if fileExists(SqliteDBPath) {
		b, err := ioutil.ReadFile(SqliteDBPath) // just pass the file name
		if err != nil {
			http.Error(w, "{}", 400)
			return
		} else {
			http.Error(w, string(b), 200)
			return
		}
	} else {
		http.Error(w, "{}", 400)
	}
}


func realInstanceCreate(body string) {

	a := &body
	stdout, err := beanstalkSend(config.BeanstalkConfig, *a)
	fmt.Printf("%s\n",stdout);

	if err != nil {
		return
	}
}

func getStructTag(f reflect.StructField) string {
	return string(f.Tag)
}

func HandleClusterCreate(w http.ResponseWriter, r *http.Request) {
	var instanceid string
	params := mux.Vars(r)
	instanceid = params["instanceid"]
	var regexpInstanceId = regexp.MustCompile(`^[aA-zZ_]([aA-zZ0-9_])*$`)
	var regexpSize = regexp.MustCompile(`^[1-9](([0-9]+)?)([m|g|t])$`)
//	var regexpEmail = regexp.MustCompile("^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$")
//	var regexpPubkey = regexp.MustCompile("^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$")
//	var regexpCallback = regexp.MustCompile(`^(http|https)://`)

	w.Header().Set("Content-Type", "application/json")

	// check the name field is between 3 to 40 chars
	if len(instanceid) < 3 || len(instanceid) > 40 {
		response := Response{"The instance name must be between 3-40"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}
	if !regexpInstanceId.MatchString(instanceid) {
		response := Response{"The instance name should be valid form, ^[aA-zZ_]([aA-zZ0-9_])*$"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}

	// check for existance
	// todo: to API
	SqliteDBPath := fmt.Sprintf("%s/var/db/bhyve/%s.sqlite", workdir,instanceid)
	if fileExists(SqliteDBPath) {
		response := Response{"vm already exist"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}

	if r.Body == nil {
		response := Response{"please send a request body"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}

        fmt.Println("create wakeup: [ %s ]", r.Body)

        var vm Vm
        _ = json.NewDecoder(r.Body).Decode(&vm)
//        json.NewEncoder(w).Encode(vm)
/*
	if ( len(vm.Email)>2 ) {
		if !regexpEmail.MatchString(vm.Email) {
			response := Response{"email should be valid form"}
			js, err := json.Marshal(response)
			if err != nil {
				http.Error(w, err.Error(), http.StatusInternalServerError)
				return
			}
			http.Error(w, string(js), 400)
			return
		}
	}

	if ( len(vm.Callback)>2) {
		if !regexpCallback.MatchString(vm.Callback) {
			response := Response{"callback should be valid form"}
			js, err := json.Marshal(response)
			if err != nil {
				http.Error(w, err.Error(), http.StatusInternalServerError)
				return
			}
			http.Error(w, string(js), 400)
			return
		}
	}

*/

	if ( len(vm.Pubkey)<30 ) {
		response := Response{"Pubkey too small"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}

	if ( len(vm.Pubkey)>500 ) {
		response := Response{"Pubkey too long"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}
//regex for pubkey
/*
	if !regexpEmail.MatchString(vm.Email) {
		response := Response{"email should be valid form"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}
*/
	uid := []byte(vm.Pubkey)
//	fmt.Printf("UID CLIENT: %x", md5.Sum(uid))

	// master value validation
	cpus, err := strconv.Atoi(vm.Cpus)
	fmt.Printf("C: [%s] [%d]\n",vm.Cpus, vm.Cpus)
	if err != nil {
		response := Response{"cpus not a number"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}
	if cpus <= 0 || cpus > 10 {
		response := Response{"Cpus valid range: 1-16"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}

	if !regexpSize.MatchString(vm.Ram) {
		response := Response{"The ram should be valid form, 512m, 1g"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}
	if !regexpSize.MatchString(vm.Imgsize) {
		response := Response{"The imgsize should be valid form, 2g, 30g"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), 400)
		return
	}

	vm.Jname = instanceid
	val := reflect.ValueOf(vm)

	var jconf_param string
	var str strings.Builder

	// of course we can use marshal here instead of string concatenation, 
	// but now this is too simple case/data without any processing
	str.WriteString("{\"Command\":\"")
	str.WriteString(runscript)
	str.WriteString("\",\"CommandArgs\":{\"mode\":\"init\",\"jname\":\"")
	str.WriteString(instanceid)
	str.WriteString("\"")

	for i := 0; i < val.NumField(); i++ {
		valueField := val.Field(i)

		typeField := val.Type().Field(i)
		tag := typeField.Tag

		tmpval := fmt.Sprintf("%s",valueField.Interface())

		if len(tmpval) == 0 {
			continue
		}

		fmt.Printf("[%s]",valueField);

		jconf_param = strings.ToLower(typeField.Name)
		if strings.Compare(jconf_param,"jname") == 0 {
			continue
		}
		fmt.Printf("jconf: %s,\tField Name: %s,\t Field Value: %v,\t Tag Value: %s\n", jconf_param, typeField.Name, valueField.Interface(), tag.Get("tag_name"))
		buf := fmt.Sprintf(",\"%s\": \"%s\"", jconf_param, tmpval)
		str.WriteString(buf)
	}

	str.WriteString("}}");
	fmt.Printf("C: [%s]\n",str.String())
//Response{string(b)}
//	response := Response{"queued"}

//	fmt.Printf("%x", h.Sum(nil))
	response := fmt.Sprintf("Cluster control CLI: ssh %x@srv-03.olevole.ru\nAPI:\ncurl -H \"cid:%x\" http://srv-03.olevole.ru:65530/api/v1/cluster\ncurl -H \"cid:%x\" http://srv-03.olevole.ru:65530/api/v1/status/%s\ncurl -H \"cid:%x\" http://srv-03.olevole.ru:65530/api/v1/destroy/%s\n", md5.Sum(uid), md5.Sum(uid), md5.Sum(uid), instanceid, md5.Sum(uid), instanceid)
//	md5uid := md5.Sum(uid)
//	response := string(md5uid[:])

//	js, err := json.Marshal(response)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	go realInstanceCreate(str.String())
//	http.Error(w, string(js), 200)
	http.Error(w, response, 200)

	return
}


func HandleClusterDestroy(w http.ResponseWriter, r *http.Request) {
	var instanceid string
	params := mux.Vars(r)
	instanceid = params["instanceid"]
	var regexpInstanceId = regexp.MustCompile(`^[aA-zZ_]([aA-zZ0-9_])*$`)

	Cid := r.Header.Get("cid")
	HomePath := fmt.Sprintf("/usr/home/%s/vms", Cid)
	//fmt.Println("CID IS: [ %s ]", cid)
	if _, err := os.Stat(HomePath); os.IsNotExist(err) {
		return
	}

	w.Header().Set("Content-Type", "application/json")

	// check the name field is between 3 to 40 chars
	if len(instanceid) < 3 || len(instanceid) > 40 {
		http.Error(w, "The instance name must be between 3-40", 400)
		return
	}
	if !regexpInstanceId.MatchString(instanceid) {
		http.Error(w, "The instance name should be valid form, ^[aA-zZ_]([aA-zZ0-9_])*$", 400)
		return
	}

	// check for existance
	// todo: to API
/*
	SqliteDBPath := fmt.Sprintf("%s/var/db/bhyve/%s.sqlite", workdir,instanceid)
	if !fileExists(SqliteDBPath) {
		response := Response{"no found"}
		js, err := json.Marshal(response)
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		http.Error(w, string(js), http.StatusNotFound)
		return
	}
*/
	// of course we can use marshal here instead of string concatenation, 
	// but now this is too simple case/data without any processing
	var str strings.Builder
	str.WriteString("{\"Command\":\"")
	str.WriteString(runscript)
	str.WriteString("\",\"CommandArgs\":{\"mode\":\"destroy\",\"jname\":\"")
	str.WriteString(instanceid)
	str.WriteString("\"")
	str.WriteString("}}");

	fmt.Printf("C: [%s]\n",str.String())
	go realInstanceCreate(str.String())

	response := Response{"destroy"}
	js, err := json.Marshal(response)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	http.Error(w, string(js), 200)
	return
}
