# PHASE 1 — gerçek IddCx kare alımı

Durum: **VERIFIED**. PHASE 2 başlatılmadı.

## Ölçülen sonuç

| Ölçüm | Sonuç |
|---|---|
| Kaynak | Etkin SweetDisplay SWT0001 indirect-display hedefi |
| Ekran modu | 2400×1080, 60/1 Hz; uzatılmış masaüstünde ayrı kaynak/konum |
| Gözlem süresi | 19,996027 s; 20 s ETW oturumu |
| Alınan kareler | 771; sürücü tanılama kimlikleri 1–771 |
| Ortalama alım hızı | 38,507650 FPS |
| Boyut / piksel biçimi | Tüm kayıtlarda 2400×1080, DXGI 87 = B8G8R8A8_UNORM |
| Kare aralığı | Ortalama 25,968866 ms; min. 8,1247 ms; maks. 55,4893 ms |
| Kimlik / acquisition-QPC sırası | Kesin artan; kimlik boşluğu 0 |
| OS presentation-frame sayacı | 269–1039; gerçek sunum güncellemeleri |
| ETW kaybı | 0 olay, 0 real-time tampon |
| Kaydedilen tanılama görüntüsü | Tam olarak 1 BMP; 10.368.054 bayt |
| Görüntü eşlemesi | Test kodu ve paint sayacı 117 pikselden çözüldü; desen günlüğüyle eşleşti |
| Üretici | ETW olay başlığındaki PID, çalışan WUDFHost.exe süreciyle eşleşti |

**Bu sonuç 60 FPS alım kapasitesini doğrulamaz.** Ekranın 60 Hz modu ile bu
GDI test penceresinin ürettiği/gözlenen güncelleme hızı ayrı ölçümlerdir.
Düşük hızın yalnızca sürücüden kaynaklandığına dair bir tanı yoktur.
Min./maks. aralıklarda tanılama ve çizim zamanlaması etkileri de vardır.

## Kanıt zinciri

1. Kaynak incelemesinde RunCore, başarılı IddCxSwapChainReleaseAndAcquireBuffer
   sonucunda yüzeyi alıp boş işlem bölümünden sonra serbest bırakıyordu.
   Bu kod tek başına gerçek piksel alımı için kayıt sağlamıyordu.
2. FrameDiagnostics::Observe tam bu başarılı alım yoluna, yüzey bırakılmadan
   ve sonraki ReleaseAndAcquire çağrısından önce eklendi.
3. ETW metadata kaydı kare kimliği, acquisition QPC, QPC frekansı, IddCx
   PresentDisplayQPCTime, aralık, boyut, biçim ve OS presentation-frame sayısını içerir.
4. Test aracı QueryDisplayConfig ve EnumDisplaySettingsEx ile etkin SWT0001
   hedefini seçti. Normal bir Win32 desen penceresini bu hedefin masaüstü
   koordinatlarına taşıdı. Renk bantları, hareketli kare, rastgele test kodu ve
   paint sayacı çizildi; paint QPC değerleri ayrıca kaydedildi.
5. Bir defalık D3D11 staging kopyası sürücüde Map(D3D11_MAP_READ) ile okundu.
   RowPitch dikkate alınarak satırlar paketlendi. ETW parçaları 48 KiB ile sınırlı;
   toplam boyut, parça sırası ve tüm piksel verisinin FNV-1a özeti alıcıda doğrulandı.
6. Alıcı test kodunu ve sayacı piksellerden çözdü. Tek BMP CREATE_NEW ile yazıldı;
   önceden mevcut dosyaya ikinci görüntü yazmayı reddeder.
7. BMP ayrıca görsel olarak incelendi: doğru renk bantları, hareketli pembe kare,
   test kodu ve 117 sayacı görünür. Tam dosyanın SHA-256 özeti özel kanıta kaydedildi.

Bu bir Desktop Duplication veya masaüstü ekran görüntüsü değildir: BMP'nin bütün
pikselleri IddCx'nin sürücümüze verdiği IDXGIResource üzerinden alınmıştır.
Test penceresi yalnızca masaüstüne desen sunar; görüntüyü alıcıya doğrudan vermez.

## Display 3 ile GDI adı

Kullanıcı çalışan temel ekranı Windows Ayarları'nda Display 3 olarak doğruladı.
Bu çalıştırmada aynı SWT0001 hedefinin GDI kaynak adı DISPLAY11 oldu.
Windows Ayarları ekran numarası ile GDI aygıt adının rakamları eşit varsayılmadı.
Hedef; SWT0001 aygıt yolu, indirect-wired çıkış türü, etkin kaynak koordinatları
ve 2400×1080@60 modu ile seçildi. İki fiziksel hedefin kimliği ve yerleşimi
özel önce/sonra topoloji kayıtlarında bulunur.

Bu turdaki ek Ayarlar incelemesi kesin bir yeni ekran-numarası teyidi sağlamadı:
önceden açık pencere eski liste gösterdi; tekrar açılan pencere kullanıcı
etkileşimi sırasında küçültüldü. Kullanıcının Display 3 teyidi ile API'den
doğrulanan SWT0001 eşlemesi ayrı kanıtlar olarak tutulur.

## Tanılama kapsamı ve maliyeti

- ETW sağlayıcısı kapalıyken CPU piksel kopyası veya kare dosyası üretimi yoktur.
- Metadata anahtar sözcüğü 1; tek-kare yakalama anahtar sözcüğü 2.
- Sürücü süreci boyunca en fazla bir yakalama girişimi vardır. Korunan yüzeyler
  okunmaz; desteklenmeyen biçim ve API/ETW hataları kodlarıyla bildirilir.
- Tek tanılama kopyası senkron GPU readback yapar; üretim hattı değildir.
- Sürekli piksel aktarımı, host IPC'si, paylaşılmış D3D kaynağı, encoder, ağ
  protokolü veya telefon kodu uygulanmadı.
- ETW oturumu normal olarak durduruldu. Desen penceresine kapanma isteği gönderildi.
- Swapchain döngüsüne sonlandırma olayını her turda kontrol etme eklendi; sürekli
  güncelleme varken de tanılama sürücüsünün güvenli kaldırılabilmesi amaçlandı.

## Tekrar üretme

EWDK bağlıyken depo kökünden:

~~~text
scripts\windows\Build-FrameProbe.cmd D:
scripts\windows\Build-FrameDiagnostics.cmd D:
~~~

Sürücü kaynak değişikliği için mevcut geliştirme sertifikası kullanılır;
ayrıntılar DEPLOYMENT_WINDOWS.md ve TEST_LOG.md içindedir. Çalışan paketi
yalnızca kaynak değişikliği gerektiğinde güncelle. Eski imzalı paket korunur.

Yönetici oturumunda, yeni ve ignored bir evidenceDir kullanarak:

~~~text
SweetDisplayFrameProbe.exe --inventory evidenceDir\display.txt
SweetDisplayFrameProbe.exe --pattern evidenceDir hexNonce
SweetDisplayFrameProbe.exe --capture evidenceDir hexNonce 20
~~~

Desen ayrı süreçte en az birkaç saniye çalışmış olmalıdır. Capture sonucunun
çıkış kodu, result.json, tüm frames.csv sırası ve desen eşlemesi birlikte kontrol
edilir. Yeni yakalama bu görevde çalıştırılmadı; mevcut tek BMP korunur.
Bir sürücü sürecinde ikinci tam-kare yakalama girişimi yapılmaz.

## Kanıt dosyaları — Git dışında

Tümü docs/evidence/private/phase1 altında, ignored:

- diagnostic-frame.bmp — tek tanılama görüntüsü.
- frames.csv — 771 kare kaydı.
- pattern.csv — bağımsız çizim sayacı/QPC günlüğü.
- result.json, verification.json, execution.json — sonuç, sıra/hash denetimi ve komutlar.
- pattern-display.txt, capture-display.txt, display-before.txt,
  display-after.txt, display-final.txt — tam yerel ekran eşlemeleri.

İmzalama ve kurulum komutları .local/phase1-update-result.json içinde; özel
sertifika/yerel yol/aygıt bilgileri public kaynaklara kopyalanmaz.

## Microsoft API dayanakları

- [IddCx buffer acquire](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/iddcx/nf-iddcx-iddcxswapchainreleaseandacquirebuffer):
  swapchain tarafından sunulan yüzeyi alma/bırakma.
- [D3D11 CopyResource](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-copyresource)
  ve [Map](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-map):
  staging readback.
- [ETW EventWrite](https://learn.microsoft.com/en-us/windows/win32/api/evntprov/nf-evntprov-eventwrite):
  boyutu sınırlı olay verisi ve hata kodları.
- [ETW olay tüketimi](https://learn.microsoft.com/en-us/windows/win32/etw/consuming-events):
  OpenTrace/ProcessTrace ile gerçek zamanlı tanılama alıcısı.

PHASE 2'nin aktarım mekanizması burada seçilmiş sayılmaz.

