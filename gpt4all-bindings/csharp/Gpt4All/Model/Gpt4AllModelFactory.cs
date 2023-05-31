﻿using Gpt4All.Bindings;
using Gpt4All.LibraryLoader;
using System.Diagnostics;

namespace Gpt4All;

public class Gpt4AllModelFactory : IGpt4AllModelFactory
{
    private static bool bypassLoading;
    private static string? libraryPath;

    private static readonly Lazy<LoadResult> libraryLoaded = new(() =>
    {
        return NativeLibraryLoader.LoadNativeLibrary(Gpt4AllModelFactory.libraryPath, Gpt4AllModelFactory.bypassLoading);
    }, true);

    public Gpt4AllModelFactory(string? libraryPath = default, bool bypassLoading = false)
    {
        Gpt4AllModelFactory.libraryPath = libraryPath;
        Gpt4AllModelFactory.bypassLoading = bypassLoading;

        if (!libraryLoaded.Value.IsSuccess)
        {
            throw new Exception($"Failed to load native gpt4all library. Error: {libraryLoaded.Value.ErrorMessage}");
        }
    }

    private static IGpt4AllModel CreateModel(string modelPath, ModelType? modelType = null)
    {
        var modelType_ = modelType ?? ModelFileUtils.GetModelTypeFromModelFileHeader(modelPath);

        var handle = modelType_ switch
        {
            ModelType.LLAMA => NativeMethods.llmodel_llama_create(),
            ModelType.GPTJ => NativeMethods.llmodel_gptj_create(),
            ModelType.MPT => NativeMethods.llmodel_mpt_create(),
            _ => NativeMethods.llmodel_model_create(modelPath),
        };

        var loadedSuccesfully = NativeMethods.llmodel_loadModel(handle, modelPath);

        if (loadedSuccesfully == false)
        {
            throw new Exception($"Failed to load model: '{modelPath}'");
        }

        var underlyingModel = LLModel.Create(handle, modelType_);

        Debug.Assert(underlyingModel.IsLoaded());

        return new Gpt4All(underlyingModel);
    }

    public IGpt4AllModel LoadModel(string modelPath) => CreateModel(modelPath, modelType: null);

    public IGpt4AllModel LoadMptModel(string modelPath) => CreateModel(modelPath, ModelType.MPT);

    public IGpt4AllModel LoadGptjModel(string modelPath) => CreateModel(modelPath, ModelType.GPTJ);

    public IGpt4AllModel LoadLlamaModel(string modelPath) => CreateModel(modelPath, ModelType.LLAMA);
}
