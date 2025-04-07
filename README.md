# hdlc_test

練習使用ai生code，發現ai在細部(bit stuffing/de-stuffing)很難做對，搞了好久還是用筆算+手刻，也順便練一下手感，好久沒寫code

- 在vscode + idf qemu可以在模擬環境下跑code，就不用燒錄esp32
- 頭尾flag(0x7e)不做stuffing/de-stuffing，只做addr，control，payload
- fcs的對象是尚未stuffing的原始的addr + control + payload
  
修改 main/CMakeLists.txt 分別產生 read or write program

write:
- org data  : 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x00, 
- fcsed data: 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x9A, 0xFD,
- hdlc data : 0x7E, 0x01, 0x03, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFB, 0xCD, 0x7D, 0x40, 0x7E, 

read:
- 把hdlc data給read當作input，扣除頭尾的flag去做de-stuffing會發現得到
- Data: 01 03 AA BB CC DD EE FF 9A FD 00
- 然後最後的00會無法判斷該不該捨棄，即他有可能是fcs(肉眼看fcs應該是9A FD)
- 網路查是說要搭配其他的protocol會有payload length可以判斷?
- 或是要從尾巴往回看，每次抓兩個byte當fcs去驗算前面的addr+control+payload來判斷fcs是哪個位置

釐清中

# gemini找到看起來可行的做法:
- 發送與接收都以bit為單位
- 對發送端，Stuffed [Address + Control + Information + FCS] 若不在byte align，那padding 0 就做在結束的FLAG後面
- 對接收端
  - 持續尋找未經 de-stuffing 的起始 Flag (01111110)
  - 找到後，開始接收數據位元，同時進行 de-stuffing（遇到 111110 就丟棄 '0'）
  - 在這個過程中，持續監測是否出現了未經 de-stuffing 的 01111110 pattern
  - 一旦檢測到這個未經 de-stuffing 的 Flag ，就確認當前幀的數據部分接收完畢（0x7E後面的padding 0丟棄）
- 以這個思路，看起來餵進de-stuffing的資料可以從頭到尾都丟進去，在裡面處理，ongoing

# 對read來說，這個方式可以達到一些目標
- 接收以bit為單位不再以byte為單位
- 開頭可以處理多個start flag(0x7E)，找到真正的payload起點，在開始de-stuffing
- 以bit為單位發送的話，fcs會接著end flag(0x7e)，表示padding 0是做在0x7e後面
- 0x7e的偵測是一直在做，payload start之後看到的第一個0x7e就是end flag
  這裡有個特別的點，看到payload會往回倒退1 byte，這樣可以避免2 pass處理整個test_frame
- 打印出來payload後清空變數，會繼續偵測start flag，繼續處理後面的frame
- 測試過故意把test_frame(整串)往右shift(故意打破byte align)，也能正確印出payload

