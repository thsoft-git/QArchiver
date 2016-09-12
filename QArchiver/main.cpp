#include <QApplication>
#include <QTextCodec>
#include "mainwindow.h"
#include "addtoarchive.h"
#include "progressdialog.h"
#include "Codecs/qibm852codec.h"
#include <QtMimeTypes/QMimeDatabase>
#include <QDebug>
#include <QMessageBox>
#include <QVariant>
#include <QElapsedTimer>

#include "getopt.h"

void parseArgs(QMap<QString, QVariant> &args, int argc, char *argv[]);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("QA");
    a.setApplicationName("QArchiver");
    a.setApplicationVersion("1.0");
    //a.setQuitOnLastWindowClosed(false);
    a.addLibraryPath("../lib");
    //QMessageBox::information(0, QLatin1String(" "), a.arguments().join(QChar('=')));

    // Install text codec cp852 (IBM852)
    new QIBM852Codec;
    //QTextCodec *codeccp852 = QTextCodec::codecForMib(2010);
    //qDebug() << "cp852: " << codeccp852->name();
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    // Výpis dostupných kodeků
    //TextEncoder::findCodecs();

    // Build database -> pote rychlejsi vyhledavani
    //QElapsedTimer timer;
    //timer.start();
    QMimeDatabase db;
    QString sufix = db.suffixForFileName("file.rar"); // Execute query to build db
    //QMimeType mt = db.mimeTypeForFile("file.tar.bz2");
    //qDebug() << mt.name() << mt.filterString() << mt.comment() << mt.preferredSuffix() << mt.suffixes();
    //qDebug() << "The searching file MimeType took" << timer.elapsed() << "milliseconds";
    Q_UNUSED(sufix);

//    for(int i = 0; i < argc; i++) {
//        // uses QTextCodec::setCodecForCStrings() If no codec has been set, this function does the same as fromLatin1()
//        qDebug() << "argv[" << i << "] = " << QString::fromAscii(argv[i]) << "Codec: "<< QTextCodec::codecForCStrings();
//        // fromLocal8Bit -> uspech, pouziva QTextCodec::codecForLocale() defaultne System codec coz je pravdepodobne win-1250
//        qDebug() << "argv[" << i << "] = " << QString::fromLocal8Bit(argv[i]); // ANSI codepage:  1250 (for cz win)
//        qDebug() << "argv[" << i << "] = " << QString::fromUtf8(argv[i]);
//        qDebug() << "argv[" << i << "] = " << QString::fromUtf16((ushort *)argv[i]);
//        qDebug() << "argv[" << i << "] = " << QString::fromUcs4((uint *)argv[i]);
//        qDebug() << "argv[" << i << "] = " << QString::fromWCharArray((wchar_t *)argv[i]); // ANSI codepage:  1250
//    }

//    qDebug() << "QApplication arguments: "<< a.arguments();

    /* opt parse */
    QMap<QString, QVariant> args;
    parseArgs(args, argc, argv);
    /* end parsing */

    if (args.contains("add") || args.contains("add-to")) {
        AddToArchive *addToArchiveJob = new AddToArchive;
        a.connect(addToArchiveJob, SIGNAL(result(QJob*)), SLOT(quit()), Qt::QueuedConnection);

        if (args.contains("add-to")) {
            addToArchiveJob->setFilename(args.value("add-to").toString());
        }

        if (args.contains("autofilename")) {
            addToArchiveJob->setAutoFilenameSuffix(args.value("autofilename").toString());
        }

        if (args.contains("file")) {
            addToArchiveJob->addFiles(args.values("file"));
        }

        if (args.contains("dialog")) {
            if (!addToArchiveJob->showAddDialog()) {
                return 0;
            }
        }

        ProgressDialog progressDlg;
        addToArchiveJob->connectDialog(&progressDlg);
        progressDlg.show();
        addToArchiveJob->start();
        return a.exec();
    }
    else {
        MainWindow w;
        w.show();
        if (args.contains("file")) {
            QString filePath = args.value("file").toString();
            qDebug() << "Trying to open:" << filePath;
            if (args.contains("dialog")) {
                w.setShowExtractDialog(true);
            }
            w.openArchive(filePath);
        }
        return a.exec();
    }
    
    //return a.exec();
}


void parseArgs(QMap<QString, QVariant> &args, int argc, char *argv[])
{
    int c;

    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            //{"verbose", no_argument,       &verbose_flag, 1},
            //{"brief",   no_argument,       &verbose_flag, 0},
            /* These options don’t set a flag. We distinguish them by their indices. */
            {"add",             no_argument,        0, 'c'},
            {"add-to",          required_argument,  0, 't'},
            {"autofilename",    required_argument,  0, 'f'},
            {"dialog",          no_argument,        0, 'd'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "ct:pf:bead", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            qDebug("option %s", long_options[option_index].name);
            if (optarg)
                qDebug (" with arg %s", optarg);
            qDebug ("\n");
            break;

        case 'c':
            qDebug ("option -c (--add)");
            args.insert("add", (int)NULL);
            break;

        case 't':
            qDebug ("option -t (--add-to) with value `%s'\n", optarg);
            args.insert("add-to", QString(optarg));
            break;

        case 'f':
            qDebug("option -f (--autofilename) with value `%s'\n", optarg);
            args.insert("autofilename", QString(optarg));
            break;

        case 'd':
            qDebug ("option -d (--dialog)");
            args.insert("dialog", (int)NULL);
            break;

        case '?':
            /* getopt_long already printed an error message. */
            break;

        default:
            abort();
        }
    }

    /* Instead of reporting ‘--verbose’
         and ‘--brief’ as they are encountered,
         we report the final status resulting from them. */
    /*if (verbose_flag)
        qDebug ("verbose flag is set");*/

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        qDebug("non-option ARGV-elements: ");
        while (optind < argc) {
            QString fileNameArg = QString::fromLocal8Bit(argv[optind++]);
            args.insertMulti("file", fileNameArg);
            qDebug("%s ", fileNameArg.toAscii().constData());
        }
        qDebug("\n");
    }
}
