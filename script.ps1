$fileDirectory = "C:\Users\rkagh\Desktop\objpool"
$fileName = "./ObjectPoolTest.exe"


Set-Location $fileDirectory

for ($i = 1; $i -le 2048; $i *= 2)
{
    powershell -Command $fileName $i
}

pause