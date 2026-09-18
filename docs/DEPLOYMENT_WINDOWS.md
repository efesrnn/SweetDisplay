# SweetDisplay Windows dağıtım planı — 2026-09-14
> PHASE 1 sonucu — 2026-09-15: gerçek IddCx kare alımı VERIFIED.
> Açık tanılama kaynak değişikliği nedeniyle yalnızca sürücü oem82.inf olarak
> güncellendi. Mevcut sertifika/güven kullanıldı; monitör oem81.inf değişmedi;
> önceki imzalı sürücü oem80.inf geri dönüş için korundu. 771 kare ve eşleşen
> tek görüntü doğrulandı. Güncel kapsam/kanıt: FRAME_RECEPTION.md ve STATUS.md.


> Son durum — 2026-09-15: D: EWDK ön kontrolü yönetici oturumunda geçti.
> Yerel sertifika/trust, staging DLL/CAT imzaları ve iki Driver Store paketi
> (oem80.inf, oem81.inf) tamamlandı. Kurulu dosyaların SignTool kontrolleri geçti.
> Yardımcı açık; PnP monitör kaydı SweetDisplay AMOLED / OK / 0.
> Ek yükseltilmemiş Get-PnpDevice sorgusunda Access denied alındığı için duruldu.
> Ayarlar adı, mod ve uzatma henüz doğrulanmadı. Ayrıntılar TEST_LOG.md içinde.

Durum: **yerel imzalama ve iki paketin Driver Store kaydı VERIFIED; ekran testi eksik.**
İlk planın ardından kullanıcı yerel kurulum için açık onay verdi. Secure Boot, TESTSIGNING,
imza denetimi, HVCI ve telefon durumu değiştirilmedi. Sertifika güveni eklendi; PnPUtil ve
SweetDisplayDevice çalıştırıldı. Kaynaklar yeniden derlenmedi; özgün paketler korundu, ayrı kopya imzalandı.

## 1. İlk incelenen paketlerin kesin sınırı (PHASE 1 tanılaması öncesi)

Aşağıdaki yollar depo köküne göredir; kişisel mutlak yollar yazılmamıştır.

**Sürücü paketi:** `out/windows/x64/Debug/SweetDisplayDriver/bin/SweetDisplayDriver/`

| Dosya | Bayt | İşlev |
|---|---:|---|
| SweetDisplayDriver.inf | 1768 | Display sınıfı PnP kurulum tanımı |
| sweetdisplaydriver.cat | 1276 | INF/DLL bütünlüğünü kapsayan katalog; henüz imzasız |
| SweetDisplayDriver.dll | 52736 | SweetDisplay UMDF sürücüsü |

INF sürümü 09/13/2026,14.44.32.951; hedef NTamd64, Windows build 22000 ve sonrası.
Kimlikler: `Root\SweetDisplayDriver` ve `SweetDisplayDriver`.
SourceDisksFiles/CopyFiles yalnızca SweetDisplayDriver.dll içeriyor.
Co-installer, özel SYS, WDF kurulum EXE'si veya başka dağıtılacak sürücü DLL'si yok.

**Ayrı monitör paketi:** `out/windows/monitor/`

| Dosya | Bayt | İşlev |
|---|---:|---|
| SweetDisplayMonitor.inf | 888 | MONITOR\SWT0001 için ad ve tercih edilen mod |
| sweetdisplaymonitor.cat | 726 | Monitör INF kataloğu; henüz imzasız |

Monitör INF sürümü 09/13/2026,0.1.0.0. Kendi DLL/SYS dosyası yok; Windows'un
monitor.inf/PnPMonitor.Install tanımını kullanır. Tam SweetDisplay AMOLED adının
Ayarlar'daki davranışı henüz sınanmadı.

**Ayrı geliştirme aracı:** `out/windows/x64/Debug/SweetDisplayDevice/bin/SweetDisplayDevice.exe`
(29696 bayt). Yazılımsal aygıtı oluşturur; iki INF'nin CopyFiles listesinde veya
kataloglarında yer almaz. Bu prototipte aygıtı oluşturmak için gerekir; Driver Store
paketinin bir dosyası değildir.

PDB, LIB, EXP, OBJ, TLOG, test EXE'si ve EWDK medyası kurulum paketine konmaz.
Driver PDB'si bin üst dizininde 1953792 bayt; Microsoft başvurusu için sembol dosyası
gerekebilir, fakat son kullanıcı kurulum bağımlılığı değildir.

### Çalışma zamanı bağımlılıkları

Dumpbin /dependents sonucu sürücü DLL'si ntdll, KERNEL32, dxgi, d3d11, AVRT ve
Windows UCRT API sözleşmelerini içe aktarıyor. Ayrı bir özel çalışma zamanı
dağıtımı tespit edilmedi. UMDF/IddCx Windows bileşenleridir.

Yardımcı Debug EXE; KERNEL32, CFGMGR32, **VCRUNTIME140D.dll ve ucrtbased.dll**
gerektiriyor. İki Debug CRT dosyası bu geliştirme makinesinde mevcut ve imzaları
doğrulandı; EXE çalıştırılmadığı için yükleme başarısı iddia edilmiyor.
Genel dağıtım için Release yardımcı araç ve bağımlılık kontrolü planlanmalı.
Debug CRT dosyaları genel dağıtılabilir VC Redistributable bileşenleri değildir;
geliştiricinin test bilgisayarları için kullanım ayrı kapsamdır.
[Microsoft Debug dağıtım açıklaması](https://learn.microsoft.com/en-us/cpp/windows/preparing-a-test-machine-to-run-a-debug-executable?view=msvc-170).

## 2. SignTool ile doğrulanan mevcut imza durumu

EWDK SignTool ürün sürümü 10.0.26100.6584 kullanıldı. Üretilen dosyalar için
`verify /pa /v`; INF/katalog bağlantıları için ayrıca `verify /pa /v /c` uygulandı.

| Hedef | Sonuç | Çıkış |
|---|---|---:|
| sweetdisplaydriver.cat | No signature found | 1 |
| SweetDisplayDriver.dll | No signature found; gömülü imza yok | 1 |
| SweetDisplayDevice.exe | No signature found; gömülü imza yok | 1 |
| sweetdisplaymonitor.cat | No signature found | 1 |
| Driver INF, driver CAT üzerinden | Katalog girdisi bulundu; katalog imzası olmadığı için güven doğrulanamadı | 1 |
| Driver DLL, driver CAT üzerinden | Katalog girdisi bulundu; katalog imzası olmadığı için güven doğrulanamadı | 1 |
| Monitor INF, monitor CAT üzerinden | Katalog girdisi bulundu; katalog imzası olmadığı için güven doğrulanamadı | 1 |

INF'nin içine PE imzası gömülmez; paket kataloğu üzerinden doğrulanır.
SignTool çıktısındaki katalog eşleşmesi, imza başarılı demek değildir.
Bu sonuçlar hash bozulması tanısı da değildir: rapor edilen hata imzanın yokluğudur.

Windows'un mevcut IndirectKmd.sys, WUDFRd.sys ve monitor.sys dosyaları
`verify /a /kp /v` ile çıkış 0 verdi. IddCx.dll, VCRUNTIME140D.dll ve
ucrtbased.dll `verify /a /pa /v` ile çıkış 0 verdi.
Bunlar yeniden imzalanmayacak veya SweetDisplay paketine kopyalanmayacak.

Ham SignTool/dumpbin gözlemleri, aktif politika sorgusu ve altı dağıtım dosyasının
SHA-256 manifesti ignored docs/evidence/private altında tutulur.
Kataloglar yerel test veya Microsoft imzasıyla değiştiğinde yeni bir
teslim manifesti almalıdır; imzalanmış dosyalar sonradan değiştirilmemelidir.

## 3. UMDF, kernel veya karma sınıflandırması

**SweetDisplay'in dağıttığı sürücü kodu UMDF-only'dir.**
INF: UmdfService=SweetDisplayDriver, UmdfLibraryVersion=2.25.0,
UmdfExtensions=IddCx0102, ServiceBinary=.../UMDF/SweetDisplayDriver.dll.
PE x64 DLL ve kullanıcı modu Windows alt sistemi olarak doğrulandı.

**Tam Windows aygıt yığını hem kullanıcı hem kernel modu Windows bileşenleri içerir:**
WUDFRd.sys reflektörü, IndirectKmd.sys üst filtresi ve monitör için monitor.sys.
Bunlar Microsoft'un işletim sistemi bileşenleridir; SweetDisplay tarafından
geliştirilmiş karma UMDF+özel-KMDF paketimiz yoktur.
UmdfKernelModeClientPolicy=AllowKernelModeClients satırı DLL'yi kernel sürücüsü yapmaz.
[IddCx modeli](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/indirect-display-driver-model-overview).

## 4. DEVELOPMENT SIGNING

### Karar ve kanıtın sınırı

**Öneri: yerel test sertifikasıyla imzalanmış UMDF/PnP paketleri; Secure Boot ON,
HVCI ON, TESTSIGNING OFF. Gerekli en küçük yapılandırma değişikliği sertifika
güvenidir; önyükleme veya HVCI değişikliği değildir.** EV sertifikası, ücretli CA,
Microsoft Hardware hesabı veya attestation başvurusu gerekmez.

Önceki plan genel kernel test akışını bu UMDF paketine fazla geniş uyguladı.
Bu bölüm o önerinin yerine geçer. Kendi kernel SYS dosyamız yok; kullanılan
Microsoft kernel bileşenlerinin imzaları doğrulandı.

| Mevcut koruma | Geliştirme planı | Gerekçe |
|---|---|---|
| Secure Boot ON | Açık kalır | İlgili kernel imzalama kısıtı user-mode sürücüleri kapsamaz |
| HVCI ON | Açık kalır | Kernel kod bütünlüğünü denetler; kendi kernel kodumuz yok |
| TESTSIGNING OFF | Kapalı kalır | PnP yayıncı güveni ile kernel test modu ayrı mekanizmalardır |
| Normal imza denetimi | Etkin kalır | Katalog güveni ve içerik bütünlüğü geçerli olmalıdır |

Bu üç ayardan birinin bu paketi engellediği doğrulanmış değildir. **Doğrulanan
eksiklik imzasız kataloglardır.** Yerel imza ve uygun sertifika güveni bu eksikliği
gideren geliştirme yoludur. DLL'yi ayrıca gömülü imzalamak planlanıyor; bu ek
bütünlük adımı HVCI'nin UMDF'ye zorunlu kıldığı bir işlem olarak sunulmuyor.

Resmî dayanaklar:

- [Secure Boot imza kapsamı](https://learn.microsoft.com/en-us/windows/compatibility/secured-boot-signing-requirements-for-kernel-mode-drivers)
  user-mode sürücüleri ilgili kernel kısıtından açıkça ayırır. Sayfa Windows 8
  kökenlidir; tek başına bu prototipin Windows 11 çalışma testi değildir.
- [User-mode imzalama açıklaması](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/windows-driver-signing-tutorial)
  user-mode binary imzasını PnP paket kurulumu gereksiniminden ayırır.
- [PnP paket güveni](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/pnp-device-installation-signing-requirements--windows-vista-and-later-)
  katalog, bütünlük ve güven zinciri gereksinimlerini açıklar.
- [HVCI kapsamı](https://learn.microsoft.com/en-us/windows-hardware/design/device-experiences/oem-hvci-enablement)
  kernel kod bütünlüğüdür; ayrı App Control politikalarıyla karıştırılmamalıdır.
- [TESTSIGNING seçeneği](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)
  kernel-mode test kodu yükleme bağlamındadır.
  [Genel test paketi kurulum sayfası](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/installing-test-signed-driver-packages)
  TESTSIGNING dahil genel yeterli koşullar verir; bu listeyi her UMDF paketi için
  zorunlu minimum olarak uygulamıyoruz.
- [Test sertifikası güveni](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/installing-a-test-certificate-on-a-test-computer):
  Trusted Root Certification Authorities ve Trusted Publishers depoları.

**Kanıt seviyesi:** resmî imzalama modelinden ve yerel paket incelemesinden çıkarım.
Bu belirli IddCx paketinin bu bilgisayarda kurulup çalıştığı henüz doğrulanmadı.
Ek App Control/kurum politikası, INF bağlanması ve runtime sorunları ayrıca
değerlendirilir. Başarısızlıkta otomatik güvenlik gevşetmesi yapılmayacak.

Hedef Windows 11 Pro 25H2 x64. Tam build özel kanıt dosyalarında tutulur.
Aktif CI sorgusu: normal CI etkin, test modu kapalı, HVCI etkin, UMCI bayrağı
kapalı; Secure Boot kayıt gözlemi etkin. Bu, tüm olası App Control kurallarının
ve gelecekteki BCD seçeneklerinin eksiksiz denetimi anlamına gelmez.

### Referans geliştirme komutları — yürütülen adımlar TEST_LOG içinde

Yalnızca ayrı onaydan sonra, aynı Windows kullanıcısının yönetici PowerShell
oturumunda, SweetDisplay depo kökünden sırayla uygulanır. Hata sonraki adımı
durdurur. İlk kurulum akışıdır; mevcut SweetDisplay kurulumu varsa önce envanter
uzlaştırılır. Bu sertifikaya güven, aynı anahtarın imzaladığı başka dosyalara
da güven oluşturabilir; anahtar kısa ömürlü, dışa aktarılamaz ve projeye özeldir.

**1. Ön kontrol ve değişkenler.** BCDEdit burada salt okunur.
Secure Boot True, HVCI aktif ve TESTSIGNING kapalı olmalıdır.

~~~powershell
$ErrorActionPreference = 'Stop'
$repo = (Get-Item -LiteralPath '.').FullName
$localDir = Join-Path $repo '.local'
$statePath = Join-Path $localDir 'development-signing.json'
$cerPath = Join-Path $localDir 'SweetDisplayDevelopment.cer'
$expectedInfs = @('SweetDisplayDriver.inf', 'SweetDisplayMonitor.inf')
$devicePattern = '^(SWD\\SweetDisplayDriver\\|ROOT\\SweetDisplayDriver\\|DISPLAY\\SWT0001\\)'
$signtool = 'D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe'
$inf2cat = 'D:\Program Files\Windows Kits\10\bin\10.0.26100.0\x86\Inf2Cat.exe'
function Invoke-Checked([string]$Executable, [string[]]$Arguments) {
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed: $LASTEXITCODE" }
}
function Get-SweetPackages {
    Get-WindowsDriver -Online | Where-Object {
        $_.ProviderName -eq 'SweetDisplay Project' -and
        (Split-Path $_.OriginalFileName -Leaf) -in $expectedInfs
    }
}
if ((Test-Path $statePath) -or (Test-Path $cerPath)) {
    throw 'Existing signing state: reconcile first.'
}
if (@(Get-SweetPackages).Count -or
    @(Get-PnpDevice | Where-Object InstanceId -match $devicePattern).Count) {
    throw 'Existing SweetDisplay installation: reconcile first.'
}
foreach ($toolPath in @($signtool, $inf2cat)) {
    if (!(Test-Path -LiteralPath $toolPath)) { throw 'EWDK tool missing.' }
}
Invoke-Checked 'bcdedit.exe' @('/enum', '{current}')
if (!(Confirm-SecureBootUEFI)) { throw 'Secure Boot is not ON.' }
$hvci = Get-ItemPropertyValue -LiteralPath 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' -Name HVCIEnabled
if ($hvci -ne 1) { throw 'HVCI is not active.' }
Get-CimInstance -Namespace 'root\Microsoft\Windows\DeviceGuard' -ClassName Win32_DeviceGuard |
    Select-Object VirtualizationBasedSecurityStatus, SecurityServicesRunning
New-Item -ItemType Directory -Path $localDir -Force | Out-Null
~~~

D: mevcut EWDK bağlama noktasıdır; farklıysa yalnızca araç yolu düzeltilir.
BCD'de testsigning yok/No olmalı. Mevcut salt okunur
SystemCodeIntegrityInformation sorgusuyla aktif test modu da tekrar doğrulanır;
ON ise durulur.

**2. Sertifika ve staging.** Anahtar CurrentUser\My içinde; yalnızca açık
sertifika dışa aktarılır. PFX üretilmez; mevcut build dosyaları değiştirilmez.

~~~powershell
$runId = [guid]::NewGuid().ToString('N')
$stage = Join-Path $repo "out\dev-signed\$runId"
$certParameters = @{
    Type = 'CodeSigningCert'
    Subject = "CN=SweetDisplay Development $runId"
    CertStoreLocation = 'Cert:\CurrentUser\My'
    KeyAlgorithm = 'RSA'
    KeyLength = 2048
    HashAlgorithm = 'SHA256'
    KeyUsage = 'DigitalSignature'
    KeyExportPolicy = 'NonExportable'
    NotAfter = (Get-Date).AddDays(90)
}
$cert = New-SelfSignedCertificate @certParameters
$thumb = $cert.Thumbprint
$state = [pscustomobject]@{
    Thumbprint = $thumb; Subject = $cert.Subject; Stage = $stage
    Packages = @(); Devices = @()
}
$state | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $statePath -Encoding utf8
Export-Certificate -Cert $cert -FilePath $cerPath | Out-Null
$driverStage = Join-Path $stage 'driver'
$monitorStage = Join-Path $stage 'monitor'
New-Item -ItemType Directory -Path $driverStage, $monitorStage | Out-Null
$driverSource = Join-Path $repo 'out\windows\x64\Debug\SweetDisplayDriver\bin\SweetDisplayDriver'
Copy-Item -LiteralPath (Join-Path $driverSource 'SweetDisplayDriver.inf') -Destination $driverStage
Copy-Item -LiteralPath (Join-Path $driverSource 'SweetDisplayDriver.dll') -Destination $driverStage
Copy-Item -LiteralPath (Join-Path $repo 'out\windows\monitor\SweetDisplayMonitor.inf') -Destination $monitorStage
$driverInf = Join-Path $driverStage 'SweetDisplayDriver.inf'
$driverDll = Join-Path $driverStage 'SweetDisplayDriver.dll'
$driverCat = Join-Path $driverStage 'sweetdisplaydriver.cat'
$monitorInf = Join-Path $monitorStage 'SweetDisplayMonitor.inf'
$monitorCat = Join-Path $monitorStage 'sweetdisplaymonitor.cat'
~~~

**3. İmzalama sırası:** DLL -> yeni kataloglar -> katalog imzaları.
INF değiştirilmez; imzadan sonra paket içeriği değiştirilmez.

~~~powershell
Invoke-Checked $signtool @('sign', '/fd', 'SHA256', '/s', 'My', '/sha1', $thumb, $driverDll)
Invoke-Checked $inf2cat @("/driver:$driverStage", '/os:10_GE_X64', '/uselocaltime')
Invoke-Checked $inf2cat @("/driver:$monitorStage", '/os:10_GE_X64', '/uselocaltime')
Invoke-Checked $signtool @('sign', '/fd', 'SHA256', '/s', 'My', '/sha1', $thumb, $driverCat)
Invoke-Checked $signtool @('sign', '/fd', 'SHA256', '/s', 'My', '/sha1', $thumb, $monitorCat)
~~~

/sha1 sertifika parmak izi seçimidir; imza özeti /fd SHA256'dır.
Kısa ömürlü geliştirme imzasında zaman damgası hizmeti kullanılmaz; sertifika
süresi dolduğunda yenilenir ve yeniden imzalanır.
[New-SelfSignedCertificate](https://learn.microsoft.com/en-us/powershell/module/pki/new-selfsignedcertificate?view=windowsserver2025-ps),
[SignTool](https://learn.microsoft.com/en-us/windows/win32/seccrypto/signtool).

**4. Sertifika güveni ve doğrulama.** Her SignTool sonucu 0 olmadan devam edilmez.

~~~powershell
Import-Certificate -FilePath $cerPath -CertStoreLocation 'Cert:\LocalMachine\Root' | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation 'Cert:\LocalMachine\TrustedPublisher' | Out-Null
foreach ($signedFile in @($driverDll, $driverCat, $monitorCat)) {
    Invoke-Checked $signtool @('verify', '/pa', '/v', $signedFile)
}
Invoke-Checked $signtool @('verify', '/pa', '/v', '/c', $driverCat, $driverInf)
Invoke-Checked $signtool @('verify', '/pa', '/v', '/c', $driverCat, $driverDll)
Invoke-Checked $signtool @('verify', '/pa', '/v', '/c', $monitorCat, $monitorInf)
Get-ChildItem -LiteralPath $driverStage, $monitorStage -File |
    Get-FileHash -Algorithm SHA256 |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $localDir 'development-signed-hashes.json') -Encoding utf8
~~~

/pa başarısı runtime başarısı değildir. Kendi SYS dosyamız olmadığı için /kp,
UMDF DLL'ye yeni bir kernel imza şartı olarak uygulanmaz.
[Import-Certificate](https://learn.microsoft.com/en-us/powershell/module/pki/import-certificate?view=windowsserver2025-ps).

**5. Driver Store ve yazılım aygıtı.** OEM adları tahmin edilmez; her denemeden
sonra gerçek adlar kaydedilir. /add-driver tek başına aygıt oluşturmaz.

~~~powershell
function Save-SweetDisplayState {
    $state.Packages = @(Get-SweetPackages | ForEach-Object {
        [pscustomobject]@{
            PublishedName = $_.Driver
            OriginalName = Split-Path $_.OriginalFileName -Leaf
        }
    })
    $state.Devices = @(Get-PnpDevice |
        Where-Object InstanceId -match $devicePattern |
        Select-Object -ExpandProperty InstanceId)
    $state | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $statePath -Encoding utf8
}
try { Invoke-Checked 'pnputil.exe' @('/add-driver', $driverInf) }
finally { Save-SweetDisplayState }
try { Invoke-Checked 'pnputil.exe' @('/add-driver', $monitorInf) }
finally { Save-SweetDisplayState }
if (@($state.Packages).Count -ne 2) { throw 'Both package registrations must be verified.' }
$helper = Join-Path $repo 'out\windows\x64\Debug\SweetDisplayDevice\bin\SweetDisplayDevice.exe'
Start-Process -FilePath $helper -WorkingDirectory (Split-Path $helper) -WindowStyle Normal
~~~

Helper etkileşimli olduğu için görünür pencere gerekir; X ile aygıtı kapatır.
SwDeviceCreate sonucunu bekle; başarısızsa dur. Başarılı callback tek başına
sürücünün başladığını kanıtlamaz. Başarıdan sonra aynı oturumda:

~~~powershell
Save-SweetDisplayState
try { Invoke-Checked 'pnputil.exe' @('/add-driver', $monitorInf, '/install') }
finally { Save-SweetDisplayState }
$devices = @(Get-PnpDevice | Where-Object InstanceId -match $devicePattern)
$devices | Format-Table Status, Class, FriendlyName, InstanceId
foreach ($device in $devices) {
    Get-PnpDeviceProperty -InstanceId $device.InstanceId -KeyName 'DEVPKEY_Device_ProblemCode'
    Invoke-Checked 'pnputil.exe' @('/enum-devices', '/instanceid', $device.InstanceId, '/drivers')
}
~~~

/install daha yüksek sıralamalı sürücüyü zorla değiştirmez. Bağlı INF ve problem
kodu doğrulanır. 3010/reboot-required dahil sıfır dışı sonuçlarda durulur ve
değerlendirilir; otomatik reboot yok. Helper açık kalmalı; servis veya başlangıç
görevi oluşturulmaz.
[PnPUtil](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/pnputil-command-syntax),
[SwDeviceCreate](https://learn.microsoft.com/en-us/windows/win32/api/swdevice/nf-swdevice-swdevicecreate).

**Başarı:** Windows Settings > System > Display içinde SweetDisplay AMOLED
uzatılabilir ekran olarak görünür; Extend seçildiğinde aktif 2400x1080 @ 60 Hz
yolu QueryDisplayConfig ile doğrulanır. Mevcut ana ekran korunur. Sadece Driver
Store kaydı veya Device Manager girdisi başarı değildir. Ekran adının sunumu
bu Windows sürümünde ayrıca test edilir. Telefon/görüntü aktarımı kapsam dışıdır.

Hata halinde ilgili zaman aralığının setupapi.dev.log, CodeIntegrity/Operational,
UMDF olayları ve problem kodları özel kanıt dizininde incelenir; güvenlik
gevşetilmez ve ham makine logları Git'e konmaz.

### Hızlı yineleme

Geçerli sertifika tekrar kullanılır; her build için yeni güven girişi gerekmez.
Helper X ile kapatılır; yeni build ayrı staging'e alınır; DLL imzası -> Inf2Cat ->
CAT imzası -> doğrulama tekrarlanır. Eski paket sıralaması güncellemeyi engellerse
INF sürümü düzgün yükseltilir veya yalnızca kayıtlı eski SweetDisplay OEM paketi
kontrollü kaldırılır; /force kullanılmaz. Değişmeyen monitör paketini her seferinde
eklemek gerekmez. Bütün OEM sürümleri yerel kayıtta tutulur.

### Geri alma — ÇALIŞTIRILMADI

Önce helper penceresinde **X**: SwDeviceClose aygıt ömrünü bitirir. Paket kaldırma
ayrı işlemdir.
[SwDeviceClose](https://learn.microsoft.com/en-us/windows/win32/api/swdevice/nf-swdevice-swdeviceclose),
[Microsoft kaldırma modeli](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/how-devices-and-driver-packages-are-uninstalled).

Aynı kullanıcıyla yönetici PowerShell'de, depo kökünden:

~~~powershell
$ErrorActionPreference = 'Stop'
$repo = (Get-Item -LiteralPath '.').FullName
$statePath = Join-Path $repo '.local\development-signing.json'
$state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
$thumb = $state.Thumbprint
if ($thumb -notmatch '^[A-Fa-f0-9]{40}$' -or
    $state.Subject -notmatch '^CN=SweetDisplay Development [a-f0-9]{32}$') {
    throw 'Unexpected certificate identity; stop.'
}
$expectedInfs = @('SweetDisplayDriver.inf', 'SweetDisplayMonitor.inf')
$devicePattern = '^(SWD\\SweetDisplayDriver\\|ROOT\\SweetDisplayDriver\\|DISPLAY\\SWT0001\\)'
function Invoke-Removal([string[]]$Arguments) {
    & pnputil.exe @Arguments
    $result = $LASTEXITCODE
    if ($result -eq 3010) { Write-Warning 'Reboot required; arrange an approved restart.' }
    elseif ($result -ne 0) { throw "PnPUtil removal failed: $result" }
}
# Reconcile exact current entries in case the last operation stopped before saving.
$currentDevices = @(Get-PnpDevice | Where-Object InstanceId -match $devicePattern |
    Select-Object -ExpandProperty InstanceId)
$ids = @(@($state.Devices) + $currentDevices | Sort-Object -Unique)
foreach ($id in ($ids | Sort-Object { if ($_ -like 'DISPLAY\*') { 0 } else { 1 } })) {
    if ($id -notmatch $devicePattern) { throw 'Unexpected device identity.' }
    if (Get-PnpDevice -InstanceId $id -ErrorAction SilentlyContinue) {
        Invoke-Removal @('/remove-device', $id)
    }
}
$currentPackages = @(Get-WindowsDriver -Online | Where-Object {
    $_.ProviderName -eq 'SweetDisplay Project' -and
    (Split-Path $_.OriginalFileName -Leaf) -in $expectedInfs
} | ForEach-Object {
    [pscustomobject]@{ PublishedName = $_.Driver; OriginalName = Split-Path $_.OriginalFileName -Leaf }
})
$packages = @(@($state.Packages) + $currentPackages | Sort-Object PublishedName -Unique)
foreach ($package in ($packages | Sort-Object {
    if ($_.OriginalName -eq 'SweetDisplayMonitor.inf') { 0 } else { 1 }
})) {
    if ($package.PublishedName -notmatch '^oem\d+\.inf$' -or
        $package.OriginalName -notin $expectedInfs) { throw 'Unexpected package identity.' }
    $present = Get-WindowsDriver -Online | Where-Object Driver -eq $package.PublishedName
    if (!$present) { continue }
    if ($present.ProviderName -ne 'SweetDisplay Project' -or
        (Split-Path $present.OriginalFileName -Leaf) -ne $package.OriginalName) {
        throw 'OEM name reused by a different package; stop.'
    }
    Invoke-Removal @('/delete-driver', $package.PublishedName, '/uninstall')
}
$remaining = @(Get-WindowsDriver -Online | Where-Object {
    $_.ProviderName -eq 'SweetDisplay Project' -and
    (Split-Path $_.OriginalFileName -Leaf) -in $expectedInfs
})
if ($remaining.Count) { throw 'SweetDisplay packages remain; rollback incomplete.' }
foreach ($store in @('Cert:\LocalMachine\TrustedPublisher', 'Cert:\LocalMachine\Root', 'Cert:\CurrentUser\My')) {
    $certPath = Join-Path $store $thumb
    if (Test-Path -LiteralPath $certPath) {
        if ((Get-Item -LiteralPath $certPath).Subject -ne $state.Subject) {
            throw 'Certificate subject mismatch; stop.'
        }
        if ($store -eq 'Cert:\CurrentUser\My') {
            Remove-Item -LiteralPath $certPath -DeleteKey
        } else { Remove-Item -LiteralPath $certPath }
    }
}
$cerPath = Join-Path $repo '.local\SweetDisplayDevelopment.cer'
if (Test-Path -LiteralPath $cerPath) {
    $publicCert = [Security.Cryptography.X509Certificates.X509Certificate2]::new($cerPath)
    $fileThumb = $publicCert.Thumbprint
    $publicCert.Dispose()
    if ($fileThumb -ne $thumb) { throw 'Unexpected exported certificate; stop.' }
    Remove-Item -LiteralPath $cerPath
}
~~~

İlk kurulum ön kontrolünde mevcut SweetDisplay olmaması bu kapsam için gereklidir.
Eksik kayıt veya beklenmeyen kimlikte durulur; ada göre toplu silme yapılmaz.
3010 varsa onaylı yeniden başlatmadan sonra kalan aygıt/paketler tekrar sorgulanır.
Staging ve özel işlem kaydı denetim için kalabilir; özel anahtar içermez.
CurrentUser\My silinirken DeleteKey özel anahtarı da kaldırır.
[Certificate provider Remove-Item](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.management/remove-item?view=powershell-7.6).

Son doğrulama:

~~~powershell
Confirm-SecureBootUEFI
Get-ItemPropertyValue -LiteralPath 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' -Name HVCIEnabled
bcdedit.exe /enum '{current}'
Get-PnpDevice | Where-Object InstanceId -match $devicePattern
Get-WindowsDriver -Online | Where-Object {
    $_.ProviderName -eq 'SweetDisplay Project' -and
    (Split-Path $_.OriginalFileName -Leaf) -in $expectedInfs
}
foreach ($store in @('Cert:\LocalMachine\TrustedPublisher', 'Cert:\LocalMachine\Root', 'Cert:\CurrentUser\My')) {
    Test-Path -LiteralPath (Join-Path $store $thumb)
}
~~~

Beklenen: Secure Boot True; HVCI 1; BCD testsigning yok/No ve aktif CI test modu
False; SweetDisplay aygıt/paket sonuçları boş; üç sertifika sorgusu False.
Sanal ekran kaldırılmış ve fiziksel ekran düzeni korunmuş olmalı.

**Normal geri almada BCDEdit yazma işlemi veya firmware değişikliği gerekmez.**
Yalnızca bu akışın dışında test modu yanlışlıkla açılmışsa, ayrı onayla:

~~~powershell
# ONLY if unexpectedly ON, and only after separate approval:
bcdedit.exe /set '{current}' testsigning off
~~~

Sonrasında onaylı yeniden başlatma ve aktif CI doğrulaması gerekir.
Secure Boot beklenmedik şekilde kapalıysa ayrı onayla firmware arayüzünde mevcut
anahtarlar korunarak etkinleştirilir; donanım menüsü bilinmeden evrensel komut
verilmez. HVCI kapalıysa ayrı onayla Windows Security > Device security >
Core isolation > Memory integrity ON ve istenen yeniden başlatma yapılır.
TPM temizleme, UEFI anahtar sıfırlama veya korumaları kapatma geri alma adımı değildir.

## 5. RELEASE SIGNING

**Gelecekteki üretim/dağıtım işi; şimdi başlatılmayacak.** Geliştirme sertifikası
son kullanıcılara dağıtılacak güven kökü değildir.

| Yol | Gereksinimler / maliyet | Hedef korumalar | Rol |
|---|---|---|---|
| Microsoft attestation | Hardware Developer Program uygunluğu, hesap/kuruluş doğrulaması, EV ilişkilendirmesi ve imzalı başvuru; CA/anahtar hizmeti maliyeti olabilir; HLK gerekmez | Secure Boot ON, HVCI ON, TESTSIGNING OFF; uyumluluk ayrıca test edilir | Gelecekte manuel Microsoft imzalı teslim; WHQL sertifikasyonu değildir |
| WHQL / HLK release | Hardware Program, EV/account koşulları, ilgili Windows için HLK testleri ve kabul edilen sonuç paketi; sertifika/test altyapısı gideri olabilir | Aynı normal hedef korumaları | Üretim sertifikasyonu ve uygun Windows Update dağıtımı |
| Kurumsal CA / doğrudan Authenticode | Uygun user-mode paket güveni ve kuruluş politikası; CA maliyeti olabilir | User-mode kapsamına ve hedef güvenine bağlı | Bu prototip için ücretli alternatif aranmıyor |
| Microsoft preproduction / kernel test akışı | Özel hedef provision etme veya kernel test güveni | Perakende güven politikasını değiştirebilir | Bu UMDF geliştirme hedefi için gereksiz |

Attestation retail Windows Update yayını sağlamaz. INF-only monitör paketinin
başvuru kabulü, Release yardımcı EXE, sembol/PDB gizliliği, teslim manifesti ve
runtime/HLK kapsamı release aşamasında incelenir. Hesap açma, satın alma veya
Microsoft başvurusu yapılmadı.

[Microsoft imzalama seçenekleri](https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/driver-signing-offerings),
[Attestation](https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/code-signing-attestation),
[Hardware Program kaydı](https://learn.microsoft.com/en-us/windows-hardware/drivers/dashboard/hardware-program-register).

## 6. Onay kapısı

Sonraki önerilen iş: bu PC'de kısa ömürlü sertifika oluşturma, açık sertifikaya
LocalMachine Root ve TrustedPublisher güveni verme, staging DLL/katalog imzaları,
iki SweetDisplay INF paketini Driver Store'a ekleme ve SweetDisplayDevice ile
geçici ekran testi. Secure Boot ve HVCI açık, TESTSIGNING kapalı kalacak.
Attestation ve telefon işlemi yok.

**Kullanıcının açık onayı olmadan değişiklik komutları çalıştırılmaz.**
Belgenin hazırlanması sertifika/driver deposu veya boot değişikliği izni değildir.
