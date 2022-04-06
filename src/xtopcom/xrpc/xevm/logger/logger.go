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
	//encodeConfig.FunctionKey = "func"

	encodeConfig.EncodeTime = zapcore.ISO8601TimeEncoder
	encodeConfig.EncodeDuration = zapcore.SecondsDurationEncoder
	encodeConfig.EncodeLevel = zapcore.CapitalLevelEncoder
	encodeConfig.EncodeCaller = zapcore.ShortCallerEncoder
	return zapcore.NewJSONEncoder(encodeConfig)
}

func getLogWriter() zapcore.WriteSyncer {
	lumberJackLogger := &lumberjack.Logger{
		Filename:   "./log/jsonrpc.log", //日志文件的位置
		MaxSize:    1024,                //在进行切割之前，日志文件的最大大小（以MB为单位）
		MaxBackups: 5,                   //保留旧文件的最大个数
		MaxAge:     10,                  //保留旧文件的最大天数
		Compress:   true,                //是否压缩/归档旧文件
	}
	return zapcore.AddSync(lumberJackLogger)
}
