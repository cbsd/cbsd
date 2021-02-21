package main

import "time"

type Comment struct {
	Command string
	JobID uint64
	Date time.Time
	CommandArgs map[string]string
}

type CommentProtocol interface {
	Decode(encodedComment []byte) (*Comment, error)
	Encode(comment *Comment) ([]byte, error)
}

type CommentProcessor interface {
	DoProcess(comment *Comment) error
}

type CbsdTask struct {
	Progress int
	ErrCode int
	Message string
}
