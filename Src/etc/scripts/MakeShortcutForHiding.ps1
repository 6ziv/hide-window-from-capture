Function MakeShortcutForHiding{
	[CmdletBinding()]
	
	Param(
		[Parameter(Position = 0)]
		[string]$ShortcutPath,

		[Parameter(Position = 1)]
		[string]$TargetPath,
		
		[Parameter(Position = 2)]
		[string]$IconLocation=[string]::Empty
	)
	
	if(!$ShortcutPath.EndsWith(".lnk")){
		$ShortcutPath = $ShortcutPath+".lnk"
	}
	
	if([string]::IsNullOrEmpty($IconLocation)){
		$IconLocation = $TargetPath
	}
	$HideToolPath  = Resolve-Path "$MyInvocation.MyCommand.Path/../../hidewindow_tool.exe"

	$WshShell = New-Object -comObject WScript.Shell
	$Shortcut = $WshShell.CreateShortcut($ShortcutPath)
	$Shortcut.IconLocation = "$IconLocation"
	$Shortcut.TargetPath = """$HideToolPath"""
	$Shortcut.Arguments = "run --executable ""$TargetPath"""
	$Shortcut.Save()
}
