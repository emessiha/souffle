/*
 * Souffle - A Datalog Compiler
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved
 * Licensed under the Universal Permissive License v 1.0 as shown at:
 * - https://opensource.org/licenses/UPL
 * - <souffle root>/licenses/SOUFFLE-UPL.txt
 */

/************************************************************************
 *
 * @file DebugReport.h
 *
 * Defines classes for creating HTML reports of debugging information.
 *
 ***********************************************************************/
#pragma once

#include "Global.h"

#include <fstream>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace souffle {

/**
 * Class representing a section of a HTML report.
 * Consists of a unique identifier, a title, a number of subsections,
 * and the HTML code for the body of the section.
 */
class DebugReportSection {
public:
    DebugReportSection(std::string id, std::string title, std::string body)
            : id(generateUniqueID(std::move(id))), title(std::move(title)), body(std::move(body)) {}

    DebugReportSection(
            std::string id, std::string title, std::vector<DebugReportSection> subsections, std::string body)
            : id(generateUniqueID(std::move(id))), title(std::move(title)),
              subsections(std::move(subsections)), body(std::move(body)) {}

    /**
     * Outputs the HTML code for the index to the given stream,
     * consisting of a link to the section body followed by a list of
     * the indices for each subsection.
     */
    void printIndex(std::ostream& out) const;

    /**
     * Outputs the HTML code for the title header to the given stream.
     */
    void printTitle(std::ostream& out) const;

    /**
     * Outputs the HTML code for the content of the section to the given
     * stream, consisting of the title header, the body text, followed
     * by the content for each subsection.
     */
    void printContent(std::ostream& out) const;

    bool hasSubsections() const {
        return !subsections.empty();
    }

private:
    std::string id;
    std::string title;
    std::vector<DebugReportSection> subsections;
    std::string body;

    static std::string generateUniqueID(std::string id) {
        static int count = 0;
        return std::move(id) + std::to_string(count++);
    }
};

/**
 * Class representing a HTML report, consisting of a list of sections.
 */
class DebugReport {
public:
    ~DebugReport() {
        while (!currentSubsections.empty()) {
            endSection("forced-closed", "Forcing end of unknown section");
        }
        if (!empty()) {
            std::ofstream debugReportStream(Global::config().get("debug-report"));
            debugReportStream << *this;
        }
    }
    bool empty() const {
        return sections.empty();
    }

    void addSection(const DebugReportSection& section) {
        if (!currentSubsections.empty()) {
            currentSubsections.top().emplace_back(section);
        } else {
            sections.emplace_back(section);
        }
    }

    void addSection(std::string id, std::string title, std::string code) {
        std::stringstream codeHTML;
        std::string escapedCode = std::move(code);
        while (true) {
            size_t i = escapedCode.find("<");
            if (i == std::string::npos) {
                break;
            }
            escapedCode.replace(i, 1, "&lt;");
        }
        codeHTML << "<pre>" << escapedCode << "</pre>\n";
        if (!currentSubsections.empty()) {
            currentSubsections.top().emplace_back(std::move(id), std::move(title), codeHTML.str());
        } else {
            sections.emplace_back(std::move(id), std::move(title), codeHTML.str());
        }
    }

    void startSection() {
        currentSubsections.emplace();
    }

    void endSection(std::string currentSectionName, std::string currentSectionTitle) {
        auto subsections = currentSubsections.top();
        currentSubsections.pop();
        addSection(DebugReportSection(currentSectionName, currentSectionTitle, subsections, ""));
    }

    /**
     * Outputs a complete HTML document to the given stream,
     * consisting of an index of all of the sections of the report,
     * followed by the content of each section.
     */
    void print(std::ostream& out) const;

    /**
     * Generate a debug report section for code (preserving formatting), with the given id and title.
     */
    static DebugReportSection getCodeSection(const std::string& id, std::string title, std::string code);

    friend std::ostream& operator<<(std::ostream& out, const DebugReport& report) {
        report.print(out);
        return out;
    }

private:
    std::vector<DebugReportSection> sections;
    std::stack<std::vector<DebugReportSection>> currentSubsections;
};

}  // end of namespace souffle
