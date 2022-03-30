package xwrap

/*
#include<wrap.h>
*/
import "C"

//convert eth tx to Top tx
func WrapEthTx(rawTx string) bool {
	if C.wrapEthTx(C.CString(rawTx)) != 0 {
		return false
	}
	return true
}
