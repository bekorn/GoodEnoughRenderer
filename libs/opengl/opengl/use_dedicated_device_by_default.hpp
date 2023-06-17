#pragma once

// To run the application with the external GPU, https://stackoverflow.com/a/39047129/2073225
extern "C"
{
__declspec(dllexport) i64 NvOptimusEnablement = 1;
__declspec(dllexport) i32 AmdPowerXpressRequestHighPerformance = 1;
}