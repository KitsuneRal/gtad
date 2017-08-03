#include "translator.h"

#include "model.h"
#include "analyzer.h"
#include "exception.h"
#include "printer.h"

#include <QtCore/QDir>

using namespace std;

enum ErrorCode
{
    _Base = TranslatorCodes,
    CannotCreateOutputDir, CannotWriteToFile
};

Translator::Translator(const QString& outputDirPath)
    : _outputDirPath(outputDirPath)
{
    if (!_outputDirPath.endsWith('/'))
        _outputDirPath.append('/');
}

TypeUsage Translator::mapType(const string& swaggerType, const string& swaggerFormat,
                              bool constRef) const
{
    TypeUsage tu =
        swaggerType == "boolean" ? TypeUsage("bool") :
        swaggerType == "integer" ?
            swaggerFormat == "int64" ? TypeUsage("std::int64_t", "<cstdint>") :
            swaggerFormat == "int32" ? TypeUsage("std::int32_t", "<cstdint>") :
            TypeUsage("int") :
        swaggerType == "number" ?
            TypeUsage(swaggerFormat == "float" ? "float" : "double") :
        swaggerType == "string" ?
            (swaggerFormat == "byte" || swaggerFormat == "binary" ?
                TypeUsage("QByteArray", "<QtCore/QByteArray>") :
            swaggerFormat == "date" ? TypeUsage("QDate", "<QtCore/QDate>") :
            swaggerFormat == "date-time" ?
                TypeUsage("QDateTime", "<QtCore/QDateTime>") :
            TypeUsage("QString", "<QtCore/QString>")) :
        swaggerType == "array" ? TypeUsage("QJsonArray", "<QtCore/QJsonArray>") :
        swaggerType == "object" ? TypeUsage("QJsonObject", "<QtCore/QJsonObject>") :
        TypeUsage("");
    if (tu.name.front() == 'Q' && constRef)
        tu.name = "const " + tu.name + '&';
    return tu;
}

TypeUsage Translator::mapArrayType(const TypeUsage& innerType, bool constRef) const
{
    TypeUsage tu =
        innerType.name == "QString" ?
            TypeUsage("QStringList", "<QtCore/QStringList>") :
        TypeUsage("QVector<" + innerType.name + ">",
                     innerType.imports, "<QtCore/QVector>");
    if (tu.name.front() == 'Q' && constRef)
        tu.name = "const " + tu.name + '&';
    return tu;
}

Model Translator::processFile(string filePath, string baseDirPath) const
{
    Model m = Analyzer(filePath, baseDirPath, *this).loadModel();
    if (!m.callClasses.empty() || !m.types.empty())
    {
        if (!m.fileDir.empty())
        {
            QDir d { _outputDirPath + m.fileDir.c_str() };
            if (!d.exists() && !d.mkpath("."))
                fail(CannotCreateOutputDir, "Cannot create output directory");
        }
        Printer(_outputDirPath.toStdString() + m.fileDir, m.filename).print(m);
    }

    return m;
}

