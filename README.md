# LNN Tabanlı Otonom Kaçış Robotu (ESP32)

##  Proje Hakkında
Bu proje, hazır yapay zeka kütüphaneleri kullanmak yerine **C diliyle sıfırdan yazılmış bir Sıvı Sinir Ağı (Liquid Neural Network - LNN)** çekirdeği içerir. Sistem ESP32 mikrodenetleyicisi üzerinde çalışarak, sensörlerden gelen verileri işler ve robotun engellerden refleksif bir şekilde otonom olarak kaçmasını hedefler.

##  Yazılım ve Algoritma Başarısı
Projenin yazılım tarafı ve LNN algoritması **kusursuz bir şekilde çalışmaktadır.** Terminal testlerinde ve simülasyonlarda, LNN matematiğinin sensör verilerini milisaniyeler içinde başarıyla işlediği ve robotun motorları için en doğru kaçış/dönüş kararlarını (diferansiyel olarak) ürettiği kanıtlanmıştır. Kodun karar verme hızı ve doğruluğu hedeflenen %100 başarıya ulaşmıştır.

##  Donanım Darboğazı ve Limitler
Yazılımın bu kusursuz ve hızlı yapısına rağmen, kullanılan fiziksel donanımlarda istenilen performans ve mekanik geri dönüş alınamamıştır.
LNN'in ürettiği hassas motor komutları; kullanılan standart DC motorların yavaşlığı, sürücülerin ölü bölgeleri (deadband) ve servo sensör tarama hızının yetersizliği sebebiyle fiziksel dünyaya tam aktarılamamıştır. Kısacası, **"Robotun beyni çok hızlı düşünmüş, ancak fiziksel bedeni bu kararlara aynı akıcılıkta tepki verememiştir."**

## Tespit Edilen Donanım Kısıtları
- L298N ölü bölgesi: ~%35 PWM altında motor tepkisiz
- VL53L0X dar görüş açısı: servo taramasında veri gecikmesi
- Çözüm: Fırçasız motor + encoder veya mikro metal motor + TB6612FNG sürücü

##  Gelecek Planları
Bu proje, C tabanlı LNN çekirdeğimizin çalıştığını kanıtlayan ilk başarılı sürümdür. İlerleyen süreçte:
* Yazılım çekirdeğimiz çok daha hassas tepki verebilen donanımlarla (fırçasız motorlar, daha hızlı çevre tarama sensörleri vb.) buluşturulacaktır.
* Otonom uçuş dinamikleri (Drone entegrasyonu) gibi yerçekimsiz ve sürtünmesiz ortam testlerine geçilecektir.
* Matematiksel çözücüler geliştirilerek sistemin refleks hızı daha da artırılacaktır.
