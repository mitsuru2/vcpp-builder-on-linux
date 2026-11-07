# Building SmallChatBot

SmallChatBot is a Windows C++17 application and is built with Visual Studio 2022
Community (Desktop development with C++) and the Windows 10 SDK.

Install vcpkg and set `VCPKG_ROOT` to its installation directory. From
`sample-app`, run `vcpkg install --triplet x64-windows` to restore
`nlohmann-json`, then open `sample-app/SmallChatBot.sln`, select `x64` and
`Debug` or `Release`, and build the solution in Visual Studio.

Before running the console application, set these environment variables:

- `SMALLCHATBOT_AZURE_ENDPOINT`, for example `https://your-resource.openai.azure.com`
- `SMALLCHATBOT_AZURE_DEPLOYMENT`, the deployed chat model name
- `SMALLCHATBOT_AZURE_API_VERSION`, optional; defaults to `2024-10-21`

Store the Azure OpenAI API key in Windows Credential Manager as a **Generic
Credential** with target `SmallChatBot/AzureOpenAI`. The console app reads that
credential at startup and never prints or logs the key. If the credential or
required endpoint/deployment configuration is missing, the app exits with an
error.

The Azure OpenAI resource and model deployment are intentionally provisioned
manually; this repository does not contain infrastructure deployment templates
or workflows. The GitHub Actions workflow builds the Visual Studio solution on
Windows.
