package main

import (
	"fmt"
	"time"
	"log"
	"strings"
	"encoding/json"
	"github.com/beanstalkd/go-beanstalk"
)

// beanstalk config struct
type BeanstalkConfig struct {
	Uri                     string  `json:"uri"`
	Tube                    string  `json:"tube"`
	ReplyTubePrefix         string  `json:"reply_tube_prefix"`
	ReconnectTimeout        int     `json:"reconnect_timeout"`
	ReserveTimeout          int     `json:"reserve_timeout"`
	PublishTimeout          int     `json:"publish_timeout"`
}

func beanstalkSend(config BeanstalkConfig, body string) (string,error) {

	amqpURI := config.Uri
	tube := config.Tube

	fmt.Printf("Calling beanstalkd: %s\n",amqpURI);

	c, err := beanstalk.Dial("tcp", amqpURI)

	if err != nil {
		log.Printf("Unable connect to beanstalkd broker:%s", err)
		return "",err
	}

	mytube := &beanstalk.Tube{Conn: c, Name: tube}
	id, err := mytube.Put([]byte(body), 1, 0, time.Duration(config.PublishTimeout)*time.Second)

	if err != nil {
		fmt.Printf("\nerr: %d\n",err)
		return "",err
	}

	callbackQueueName := fmt.Sprintf("%s%d",config.ReplyTubePrefix,id)
	fmt.Printf("got id: %d,callback queue name: %s\n",id,callbackQueueName)

	c1 := make(chan string)

	go func() {

		// todo: global timeout
		for {
			c.TubeSet = *beanstalk.NewTubeSet(c, callbackQueueName)
			id, body, err := c.Reserve(time.Duration(config.ReserveTimeout) * time.Second)

			if err != nil {
				fmt.Printf("\nid: %d, res: %s\n",id, err.Error())
			}
 
			if id == 0 {
				return // timeout
//				continue
			}

			cbsdTask := CbsdTask{}
			err = json.Unmarshal(body, &cbsdTask)
			if err != nil {
				log.Printf("json decode error %s", err.Error())
				c.Delete(id)
				return
			}

			if cbsdTask.Progress==100 {
				c1 <- cbsdTask.Message
			}
			c.Delete(id)
		}
	}()

	select {
		case msg1 := <-c1:
			if strings.Compare(msg1,"EOF") == 0 {
				fmt.Printf("EXIT\n");
				c.Close()
				return "",err
			} else {
				fmt.Println("received:", msg1)
				fmt.Printf("EXIT\n");
				c.Close()
				return msg1,err
			}
	}

	c.Close()
	return "",err
}
