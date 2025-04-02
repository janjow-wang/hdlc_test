# hdlc_test

練習使用ai生code，發現ai在細部(bit stuffing/de-stuffing)很難做對，搞了好久還是用筆算+手刻，也順便練一下手感，好久沒寫code @@

- 在vscode + idf qemu可以在模擬環境下跑code，就不用燒錄esp32
- 頭尾flag(0x7e)不做stuffing/de-stuffing，只做addr，control，payload
- fcs的對象是尚未stuffing的原始的addr + control + payload
  
修改 main/CMakeLists.txt 分別產生 read or write program

write:
- org data  : 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00, 
- fcsed data: 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x9A, 0xFD,
- hdlc data : 0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x40, 0x7E, 

把hdlc data給read當作input，扣除頭尾的flag去做de-stuffing會發現得到
- Data: 01 03 AA BB CC DD EE FF 9A FD 00
- 然後最後的00會無法判斷該不該捨棄，即他有可能是fcs(肉眼看fcs應該是9A FD)
- 網路查是說要搭配其他的protocol會有payload length可以判斷?
- 或是要從尾巴每次抓兩個byte當fcs去驗算前面的addr+control+payload來判斷fcs是哪個位置

釐清中


