Add-Type -AssemblyName System.Drawing
function New-PorcelainBmp([int]$w, [int]$h, [string]$path, [bool]$large) {
    $bmp = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = 'AntiAlias'
    $g.Clear([System.Drawing.Color]::FromArgb(247,246,243))
    $rnd = New-Object System.Random(7)
    for ($i = 0; $i -lt 42; $i++) {
        $px = 2 + ($rnd.Next(0,2) * 2)
        $rb = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(224,85,44))
        $alpha = 36 + $rnd.Next(0,52)
        $rb.Color = [System.Drawing.Color]::FromArgb($alpha,224,85,44)
        $g.FillRectangle($rb, $rnd.Next(0,$w), $rnd.Next(0,$h), $px, $px * (1 + $rnd.Next(0,2)))
        $rb.Dispose()
    }
    $accent = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(224,85,44))
    $ink    = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(42,42,40))
    $muted  = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(107,104,98))
    $d = [Math]::Min($w,$h) * 0.30
    $g.FillEllipse($accent, [float](($w - $d) / 2), [float]($h * 0.10), [float]$d, [float]$d)
    if ($large) {
        $sf = New-Object System.Drawing.StringFormat
        $sf.Alignment = 'Center'
        $f1 = New-Object System.Drawing.Font('Segoe UI', 13, [System.Drawing.FontStyle]::Bold)
        $f2 = New-Object System.Drawing.Font('Segoe UI', 7.5)
        $g.DrawString("VOCAL`nSIBILANCE", $f1, $ink, (New-Object System.Drawing.RectangleF(0, [float]($h * 0.42), $w, 90)), $sf)
        $g.DrawString('BYDARARARA', $f2, $muted, (New-Object System.Drawing.RectangleF(0, [float]($h * 0.78), $w, 30)), $sf)
        $f1.Dispose(); $f2.Dispose()
    }
    $g.Dispose()
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $bmp.Dispose()
    Write-Output "wrote $path"
}
New-PorcelainBmp 164 314 'packaging\wizard-large.bmp' $true
New-PorcelainBmp 55 58 'packaging\wizard-small.bmp' $false
