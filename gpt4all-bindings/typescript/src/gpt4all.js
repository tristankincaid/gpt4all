"use strict";

/// This file implements the gpt4all.d.ts file endings.
/// Written in commonjs to support both ESM and CJS projects.
const { existsSync } = require("fs");
const path = require("node:path");
const { LLModel } = require("node-gyp-build")(path.resolve(__dirname, ".."));
const {
    retrieveModel,
    downloadModel,
    appendBinSuffixIfMissing,
} = require("./util.js");
const { DEFAULT_DIRECTORY, DEFAULT_LIBRARIES_DIRECTORY } = require("./config.js");

async function loadModel(modelName, options = {}) {
    const loadOptions = {
        modelPath: DEFAULT_DIRECTORY,
        librariesPath: DEFAULT_LIBRARIES_DIRECTORY,
        allowDownload: true,
        verbose: true,
        ...options,
    };

    await retrieveModel(modelName, {
        modelPath: loadOptions.modelPath,
        allowDownload: loadOptions.allowDownload,
        verbose: loadOptions.verbose,
    });

    const libSearchPaths = loadOptions.librariesPath.split(";");

    let libPath = null;

    for (const searchPath of libSearchPaths) {
        if (existsSync(searchPath)) {
            libPath = searchPath;
            break;
        }
    }
    if(!libPath) {
        throw Error("Could not find a valid path from " + libSearchPaths);
    }
    const llmOptions = {
        model_name: appendBinSuffixIfMissing(modelName),
        model_path: loadOptions.modelPath,
        library_path: libPath,
    };

    if (loadOptions.verbose) {
        console.log("Creating LLModel with options:", llmOptions);
    }
    const llmodel = new LLModel(llmOptions);

    return llmodel;
}

function createPrompt(messages, hasDefaultHeader, hasDefaultFooter) {
    let fullPrompt = [];

    for (const message of messages) {
        if (message.role === "system") {
            const systemMessage = message.content;
            fullPrompt.push(systemMessage);
        }
    }
    if (hasDefaultHeader) {
        fullPrompt.push(`### Instruction: The prompt below is a question to answer, a task to complete, or a conversation to respond to; decide which and write an appropriate response.`);
    }
    let prompt = "### Prompt:";
    for (const message of messages) {
        if (message.role === "user") {
            const user_message = message["content"];
            prompt += user_message;
        }
        if (message["role"] == "assistant") {
            const assistant_message = "Response:" + message["content"];
            prompt += assistant_message;
        }
    }
    fullPrompt.push(prompt);
    if (hasDefaultFooter) {
        fullPrompt.push("### Response:");
    }

    return fullPrompt.join('\n');
}


function createEmbedding(llmodel, text) {
    return llmodel.embed(text)
}
async function createCompletion(
    llmodel,
    messages,
    options = {
        hasDefaultHeader: true,
        hasDefaultFooter: false,
        verbose: true,
    }
) {
    //creating the keys to insert into promptMaker.
    const fullPrompt = createPrompt(
        messages,
        options.hasDefaultHeader ?? true,
        options.hasDefaultFooter ?? true
    );
    if (options.verbose) {
        console.log("Sent: " + fullPrompt);
    }
    const promisifiedRawPrompt = llmodel.raw_prompt(fullPrompt, options, (s) => {});
    return promisifiedRawPrompt.then((response) => {
        return {
            llmodel: llmodel.name(),
            usage: {
                prompt_tokens: fullPrompt.length,
                completion_tokens: response.length, //TODO
                total_tokens: fullPrompt.length + response.length, //TODO
            },
            choices: [
                {
                    message: {
                        role: "assistant",
                        content: response,
                    },
                },
            ],
        };
    });
}

function createTokenStream() {
    throw Error("This API has not been completed yet!")
}

module.exports = {
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_DIRECTORY,
    LLModel,
    createCompletion,
    createEmbedding,
    downloadModel,
    retrieveModel,
    loadModel,
    createTokenStream
};
