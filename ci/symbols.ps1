#arg[0] = local path to source project
#arg[1] = name of the output folder
#arg[2] = github username where the repository exists (typically "stream-labs")
#arg[3] = name of the repository
#arg[4] = branch of repository
#arg[5] = AWS_ACCESS_KEY_ID
#arg[6] = AWS_SECRET_ACCESS_KEY

if ($args.count -lt 7)
{
	Write-Error "This script needs 7 arguements"
	exit
}

Write-Output ""
Write-Output "Running script..."
Write-Output "  args0 = $($args[0])"
Write-Output "  args1 = $($args[1])"
Write-Output "  args2 = $($args[2])"
Write-Output "  args3 = $($args[3])"
Write-Output "  args4 = $($args[4])"

Write-Output ""
Write-Output "Installing debugging tools from winsdksetup..."
Invoke-WebRequest https://go.microsoft.com/fwlink/?linkid=2173743 -OutFile winsdksetup.exe;    
start-Process winsdksetup.exe -ArgumentList '/features OptionId.WindowsDesktopDebuggers /uninstall /q' -Wait;    
start-Process winsdksetup.exe -ArgumentList '/features OptionId.WindowsDesktopDebuggers /q' -Wait;    
Remove-Item -Force winsdksetup.exe;

$localSourceDir = $args[0]
$outputFolder = $args[1]
$userId = $args[2]
$repository = $args[3]
$branch = $args[4]
$AWS_ACCESS_KEY_ID = $args[5]
$AWS_SECRET_ACCESS_KEY = $args[6]

# Copy symbols from the top of the source directory, it will search recursively for all *.pdb files
Write-Output ""
Write-Output "Copying symbols recursively from source directory..."

$symbolsFolder = "symbols_tempJ1M39VNNDF"
$dbgToolsPath = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64"

cmd /c rmdir $symbolsFolder /s /q
cmd /c mkdir $symbolsFolder
cmd /c .\copy_all_pdbs_recursive.cmd $localSourceDir $symbolsFolder


# Compile a list of submodules to the format (subModule_UserName, subModule_RepoName, subModule_Branch)
$mainRepoUri = "https://api.github.com/repos/$userId/$repository"
$webRequestUri = "$mainRepoUri/contents?ref=$branch"
Write-Output ""
Write-Output "Compiling a list of submodules from '$webRequestUri'..."
$mainRepoContentJson = (Invoke-WebRequest $webRequestUri -UseBasicParsing | ConvertFrom-Json)

$subModules = @(@())

foreach ($subBlob in $mainRepoContentJson)
{
	if (!$subBlob.git_url.StartsWith($mainRepoUri))
	{
		$subModule_UserName = ($subBlob.url -replace "https://api.github.com/repos/*","").split('/')[0] 		
		$subModule_RepoName = $subBlob.name
		$subModule_Branch = $subBlob.sha		
		$subModules += ,@($subModule_UserName, $subModule_RepoName, $subModule_Branch)
		
		Write-Output "Found sub-module: https://github.com/$subModule_UserName/$subModule_RepoName/tree/$subModule_Branch"
	}
}

Write-Output ""
Write-Output "Launching github-sourceindexer.ps1..."

.\github-sourceindexer.ps1 -ignoreUnknown -sourcesroot $localSourceDir -dbgToolsPath $dbgToolsPath -symbolsFolder $symbolsFolder -userId $userId -repository $repository -branch $branch -subModules $subModules -verbose

# Run symstore on all of the .pdb's
Write-Output ""
Write-Output "Running symstore on all of the .pdb's..."

cmd /c rmdir $outputFolder /s /q
cmd /c mkdir $outputFolder
cmd /c "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\symstore.exe" add /compress /r /f $symbolsFolder /s $outputFolder /t SLOBS

# Upload to aws
Write-Output ""
Write-Output "Upload to aws..."
.\s3upload.ps1 $AWS_ACCESS_KEY_ID $AWS_SECRET_ACCESS_KEY $symbolsFolder

# Cleanup
Write-Output ""
Write-Output "Cleanup after symbol script..."
cmd /c rmdir $outputFolder /s /q
cmd /c rmdir $symbolsFolder /s /q

Write-Output ""
Write-Output "Symbol script finish."