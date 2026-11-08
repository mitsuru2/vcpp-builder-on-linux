# Building SmallChatBot

SmallChatBot is a Windows C++17 application and is built with Visual Studio 2026
Community (Desktop development with C++) and the Windows 10 SDK.

Install vcpkg and set `VCPKG_ROOT` to its installation directory. From
`sample-app`, run `vcpkg install --triplet x64-windows` to restore
`nlohmann-json`, then open `sample-app/SmallChatBot.sln`, select `x64` and
`Debug` or `Release`, and build the solution in Visual Studio.

The console application uses these defaults if the environment variables are
unset. Set the variables to override them with your Azure OpenAI resource and
deployment:

- `SMALLCHATBOT_AZURE_ENDPOINT`, default: `https://smallchatbot.openai.azure.com`
- `SMALLCHATBOT_AZURE_DEPLOYMENT`, default: `gpt-4o-mini`
- `SMALLCHATBOT_AZURE_API_VERSION`, optional; defaults to `2024-10-21`

Store the Azure OpenAI API key in Windows Credential Manager as a **Generic
Credential** with target `SmallChatBot/AzureOpenAI`. The console app reads that
credential at startup and never prints or logs the key. If the credential or
API key is missing, the app exits with an error. The endpoint and deployment
defaults are examples and must match an actual Azure OpenAI resource for chat
requests to succeed.

For HTTP errors, the console displays the status and any details returned by
the service. For a 404, check that the endpoint, deployment name, and API
version match your Azure OpenAI resource. Error details may contain information
from the service; review them before sharing logs.

Console input and output support Unicode. Redirected standard input must be
UTF-8; redirected standard output and standard error are written as UTF-8.

To run the service tests from a Visual Studio Developer PowerShell, after
restoring the vcpkg dependencies, run the following from the repository root:

```powershell
cl /EHsc /std:c++17 /Isample-app/ChatBot.Core/include /Isample-app/ChatBot.Model/include `
  /Isample-app/vcpkg_installed/x64-windows/include `
  sample-app/tests/AzureOpenAIChatServiceTests.cpp `
  sample-app/ChatBot.Core/src/AzureOpenAIChatService.cpp `
  sample-app/ChatBot.Model/src/ChatMessage.cpp /Fe:AzureOpenAIChatServiceTests.exe
./AzureOpenAIChatServiceTests.exe
```

The Azure OpenAI resource and model deployment are intentionally provisioned
manually; this repository does not contain infrastructure deployment templates
or workflows. The GitHub Actions workflow builds the Visual Studio solution on
Windows.
