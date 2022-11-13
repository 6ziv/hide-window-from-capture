. .\SearchForInstalledProgram.ps1
. .\MakeShortcutForHiding.ps1

$ExpectedProgramName = "腾讯QQ"
$DisplayedProgramName = "QQ"
$SubPath = "Bin\QQ.exe"
$Installation = SearchForInstalledProgram "$ExpectedProgramName"

if($Installation -eq $null)
{
	echo "Cannot find $DisplayedProgramName installation in current-user scope."
	if (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] 'Administrator')) {
		echo "Run this as administrator to search in local-machine scope."
		Exit
	}else{
		echo "Searching in local-machine scope."
	}
	
	$Installation = SearchForInstalledProgram "$ExpectedProgramName" -SearchLocalMachine
	$UseUserDesktop = $false
}else{
	$UseUserDesktop = $true
}
if($Installation -eq $null)
{
	echo "Cannot find $DisplayedProgramName installation."
}

if ($UseUserDesktop){
	$DesktopPath = [Environment]::GetFolderPath("DesktopDirectory")
}else{
	$DesktopPath = [Environment]::GetFolderPath("CommonDesktopDirectory")
}
$InstallLocation = $Installation.InstallProperty("InstallLocation")

$TargetShortcut = "$DesktopPath\$DisplayedProgramName(hidden).lnk"
$TargetExecutable = Resolve-Path "$InstallLocation\$SubPath"
MakeShortcutForHiding "$TargetShortcut" "$TargetExecutable"
