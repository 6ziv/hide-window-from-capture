Function SearchForInstalledProgram{
	[CmdletBinding()]
	
	Param(
		[Parameter(Position = 0)]
		[string]$ProgramName,

		[Switch]$SearchLocalMachine
	)
	$installer = New-Object -ComObject WindowsInstaller.Installer
	if($SearchLocalMachine){
		$tmp = 4
	}else{
		$tmp = 3
	}
	$products = $installer.ProductsEx([String]::Empty, [String]::Empty, $tmp, 0)
	foreach ($product In $products) {
		if($product.InstallProperty('ProductName') -eq "$ProgramName"){
			return $product
		}
	}	
	return $null
}
