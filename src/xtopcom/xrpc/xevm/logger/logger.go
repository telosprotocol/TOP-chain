package logger

import (
	"github.com/natefinch/lumberjack"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var SugarLogger *zap.SugaredLogger

func InitLogger() {
	writeSyncer := getLogWriter()
	encoder := getEncoder()
	core := zapcore.NewCore(encoder, writeSyncer, zapcore.DebugLevel)

	logg := zap.New(core)
	SugarLogger = logg.Sugar()
}

func getEncoder() zapcore.Encoder {
	encodeConfig := zap.NewProductionEncoderConfig()
	encodeConfig.LevelKey = "level"
	encodeConfig.MessageKey = "msg"
	encodeConfig.TimeKey = "time"
	encodeConfig.NameKey = "name"
	encodeConfig.CallerKey = "caller"

	encodeConfig.EncodeTime = zapcore.ISO8601TimeEncoder
	encodeConfig.EncodeDuration = zapcore.SecondsDurationEncoder
	encodeConfig.EncodeLevel = zapcore.CapitalLevelEncoder
	encodeConfig.EncodeCaller = zapcore.ShortCallerEncoder
	return zapcore.NewJSONEncoder(encodeConfig)
}

func getLogWriter() zapcore.WriteSyncer {
	lumberJackLogger := &lumberjack.Logger{
		Filename:   "./log/jsonrpc.log", //log file name
		MaxSize:    1024,                //one log file max size MB
		MaxBackups: 5,                   //the latest 5 log files
		MaxAge:     10,                  //keep maximum time
		Compress:   true,                //whether compress
	}
	return zapcore.AddSync(lumberJackLogger)
}
