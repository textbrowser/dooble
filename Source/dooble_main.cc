/*
** Copyright (c) 2008 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Dooble without specific prior written permission.
**
** DOOBLE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** DOOBLE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QDir>
#include <QSplashScreen>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
#if defined(DOOBLE_REGISTER_GOPHER_SCHEME) ||	\
    defined(DOOBLE_REGISTER_JAR_SCHEME)
#include <QWebEngineUrlScheme>
#endif
#endif

extern "C"
{
#if defined(Q_OS_FREEBSD)
#include <sys/stat.h>
#endif
#if defined(Q_OS_MACOS)
#include <sys/resource.h>
#endif
}

#ifdef Q_OS_MACOS
#include "CocoaInitializer.h"
#endif
#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_aes256.h"
#include "dooble_application.h"
#include "dooble_certificate_exceptions.h"
#include "dooble_certificate_exceptions_menu_widget.h"
#include "dooble_charts.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_downloads.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_random.h"
#include "dooble_search_engines_popup.h"
#include "dooble_style_sheet.h"
#include "dooble_threefish256.h"
#include "dooble_ui_utilities.h"
#include "dooble_xchacha20.h"

#include <csignal>
#include <iostream>

QString dooble::s_google_translate_url = "";

static void signal_handler(int signal_number)
{
  /*
  ** _Exit() and _exit() may be safely called from signal handlers.
  */

  static int fatal_error = 0;

  if(fatal_error)
    _Exit(signal_number);

  fatal_error = 1;
  _Exit(signal_number);
}

int main(int argc, char *argv[])
{
  qputenv("QT_ENABLE_REGEXP_JIT", "0");
  qputenv("QV4_FORCE_INTERPRETER", "1");

  QList<QUrl> urls;
  QString screen_mode("");
  auto attach = false;
  auto disable_javascript = false;
  auto test_aes = false;
  auto test_aes_performance = false;
  auto test_threefish = false;
  auto test_threefish_performance = false;
  auto test_xchacha20 = false;
  int reload_periodically = -1;

  for(int i = 1; i < argc; i++)
    if(argv && argv[i])
      {
	if(strcmp(argv[i], "--attach") == 0)
	  attach = true;
	else if(strcmp(argv[i], "--disable-javascript") == 0)
	  disable_javascript = true;
	else if(strcmp(argv[i], "--executable-current-url") == 0)
	  i += 1;
	else if(strcmp(argv[i], "--full-screen") == 0)
	  screen_mode = "full-screen";
	else if(strcmp(argv[i], "--help") == 0)
	  {
	    qDebug() << "Dooble";
	    qDebug() << " --attach";
	    qDebug() << " --disable-javascript";
	    qDebug() << " --executable-current-url PROGRAM";
	    qDebug() << " --full-screen";
	    qDebug() << " --help";
	    qDebug() << " --listen";
	    qDebug() << " --load-url URL";
	    qDebug() << " --normal-screen";
	    qDebug() << " --private";
	    qDebug() << " --reload-periodically 15, 30, 45, 60";
	    qDebug() << " --test-aes";
	    qDebug() << " --test-aes-performance";
	    qDebug() << " --test-threefish";
	    qDebug() << " --test-threefish-performance";
	    qDebug() << " --test-xchacha20";
	    return EXIT_SUCCESS;
	  }
	else if(strcmp(argv[i], "--load-url") == 0)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      {
		QFileInfo const file_info(argv[i]);
		QUrl url;

		if(file_info.isReadable())
		  url = QUrl::fromUserInput(file_info.absoluteFilePath());
		else
		  {
		    url = QUrl::fromUserInput(argv[i]);
		    url.setScheme("https");
		  }

		if(dooble_ui_utilities::allowed_url_scheme(url))
		  urls << url;
	      }
	  }
	else if(strcmp(argv[i], "--normal-screen") == 0)
	  screen_mode = "normal-screen";
	else if(strcmp(argv[i], "--reload-periodically") == 0)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      {
		QString const a(argv[i]);
		auto ok = false;

		reload_periodically = a.toInt(&ok);
		reload_periodically = ok ? reload_periodically : -1;
	      }
	  }
	else if(strcmp(argv[i], "--test-aes") == 0)
	  test_aes = true;
	else if(strcmp(argv[i], "--test-aes-performance") == 0)
	  test_aes_performance = true;
	else if(strcmp(argv[i], "--test-threefish") == 0)
	  test_threefish = true;
	else if(strcmp(argv[i], "--test-threefish-performance") == 0)
	  test_threefish_performance = true;
	else if(strcmp(argv[i], "--test-xchacha20") == 0)
	  test_xchacha20 = true;
	else if(strcmp(argv[i], "-style") == 0)
	  i += 1;
	else
	  {
	    auto url(QUrl::fromUserInput(argv[i]));

	    if(url.isValid() == false)
	      {
		QFileInfo const file_info(argv[i]);

		if(file_info.isReadable())
		  url = QUrl::fromUserInput(file_info.absoluteFilePath());
	      }
	    else if(url.scheme() == "http")
	      url.setScheme("https");

	    if(dooble_ui_utilities::allowed_url_scheme(url))
	      urls << url;
	  }
      }

  if(test_aes)
    {
      dooble_aes256::test1();
      dooble_aes256::test1_encrypt_block();
      dooble_aes256::test1_decrypt_block();
      dooble_aes256::test1_key_expansion();
    }

  if(test_aes_performance)
    dooble_aes256::test_performance();

  if(test_threefish)
    {
      dooble_threefish256::test1();
      dooble_threefish256::test2();
      dooble_threefish256::test3();
    }

  if(test_threefish_performance)
    dooble_threefish256::test_performance();

  if(test_xchacha20)
    {
      {
	// https://www.rfc-editor.org/rfc/rfc8439

	QList<QByteArray> key;
	QList<QByteArray> nonce;
	QList<QByteArray> result;
	const uint32_t counter[] = {1, 2};

	key << QByteArray::fromHex("000102030405060708090"
				   "a0b0c0d0e0f1011121314"
				   "15161718191a1b1c1d1e1f");
	key << QByteArray::fromHex("00ff00000000000000000"
				   "000000000000000000000"
				   "0000000000000000000000");
	nonce << QByteArray::fromHex("000000090000004a00000000");
	nonce << QByteArray::fromHex("000000000000000000000000");
	result << QByteArray("10f1e7e4d13b5915500fdd1fa32071c4"
			     "c7d1f4c733c068030422aa9ac3d46c4e"
			     "d2826446079faa0914c2d705d98b02a2"
			     "b5129cd1de164eb9cbd083e8a2503c4e");
	result << QByteArray("72d54dfbf12ec44b362692df94137f32"
			     "8fea8da73990265ec1bbbea1ae9af0ca"
			     "13b25aa26cb4a648cb9b9d1be65b2c09"
			     "24a66c54d545ec1b7374f4872e99f096");

	for(int i = 0; i < key.size(); i++)
	  {
	    qDebug() << "ChaCha20 Block Computed: "
		     << dooble_xchacha20::chacha20_block(key[i],
							 nonce[i],
							 counter[i]).toHex()
		     << ".";
	    qDebug() << "ChaCha20 Block Expected: "
		     << result[i]
		     << ".";
	  }

      }

      {
	// https://datatracker.ietf.org/doc/html/rfc8439

	QList<QByteArray> key;
	QList<QByteArray> nonce;
	QList<QByteArray> plaintext;
	QList<QByteArray> result;
	const uint32_t counter[] = {1, 42};

	key << QByteArray::fromHex("000102030405060708090"
				   "a0b0c0d0e0f1011121314"
				   "15161718191a1b1c1d1e1f");
	key << QByteArray::fromHex("1c9240a5eb55d38af3338"
				   "88604f6b5f0473917c140"
				   "2b80099dca5cbc207075c0");
	nonce << QByteArray::fromHex("000000000000004a00000000");
	nonce << QByteArray::fromHex("000000000000000000000002");
	plaintext << QByteArray("Ladies and Gentlemen of the class of '99: "
				"If I could offer you only one tip for "
				"the future, sunscreen would be it.");
	plaintext << QByteArray::fromHex("2754776173206272696c6c69672c2061"
					 "6e642074686520736c6974687920746f"
					 "7665730a446964206779726520616e64"
					 "2067696d626c6520696e207468652077"
					 "6162653a0a416c6c206d696d73792077"
					 "6572652074686520626f726f676f7665"
					 "732c0a416e6420746865206d6f6d6520"
					 "7261746873206f757467726162652e");
	result << "6e2e359a2568f98041ba0728dd0d6981"
	          "e97e7aec1d4360c20a27afccfd9fae0b"
	          "f91b65c5524733ab8f593dabcd62b357"
	          "1639d624e65152ab8f530c359f0861d8"
	          "07ca0dbf500d6a6156a38e088a22b65e"
	          "52bc514d16ccf806818ce91ab7793736"
	          "5af90bbf74a35be6b40b8eedf2785e42"
	          "874d";
	result << "62e6347f95ed87a45ffae7426f27a1df"
	          "5fb69110044c0d73118effa95b01e5cf"
	          "166d3df2d721caf9b21e5fb14c616871"
	          "fd84c54f9d65b283196c7fe4f60553eb"
	          "f39c6402c42234e32a356b3e764312a6"
	          "1a5532055716ead6962568f87d3f3f77"
	          "04c6a8d1bcd1bf4d50d6154b6da731b1"
	          "87b58dfd728afa36757a797ac188d1";

	for(int i = 0; i < key.size(); i++)
	  {
	    qDebug() << "ChaCha20 Cipher Computed: "
		     << dooble_xchacha20::chacha20_encrypt(key[i],
							   nonce[i],
							   plaintext[i],
							   counter[i]).toHex()
		     << ".";
	    qDebug() << "ChaCha20 Cipher Expected: "
		     << result[i]
		     << ".";
	  }
      }

      {
	// https://datatracker.ietf.org/doc/html/draft-arciszewski-xchacha-03

	auto const key
	  (QByteArray::fromHex("000102030405060708090"
			       "a0b0c0d0e0f1011121314"
			       "15161718191a1b1c1d1e1f"));
	auto const nonce
	  (QByteArray::fromHex("000000090000004a0000000031415927"));

	qDebug() << "HChaCha20 Block Computed: "
		 << dooble_xchacha20::hchacha20_block(key, nonce).toHex()
		 << ".";
	qDebug() << "HChaCha20 Block Expected: "
		 << "\"82413b4227b27bfed30e42508a877d73"
	            "a0f9e4d58a74a853c12ec41326d3ecdc\""
		 << ".";
      }

      {
	// https://datatracker.ietf.org/doc/html/draft-arciszewski-xchacha-03

	QByteArray const result
	  ("7d0a2e6b7f7c65a236542630294e063b7ab9b555a5d5149aa21e4ae1e4fbce87"
	   "ecc8e08a8b5e350abe622b2ffa617b202cfad72032a3037e76ffdcdc4376ee05"
 	   "3a190d7e46ca1de04144850381b9cb29f051915386b8a710b8ac4d027b8b050f"
	   "7cba5854e028d564e453b8a968824173fc16488b8970cac828f11ae53cabd201"
	   "12f87107df24ee6183d2274fe4c8b1485534ef2c5fbc1ec24bfc3663efaa08bc"
	   "047d29d25043532db8391a8a3d776bf4372a6955827ccb0cdd4af403a7ce4c63"
	   "d595c75a43e045f0cce1f29c8b93bd65afc5974922f214a40b7c402cdb91ae73"
	   "c0b63615cdad0480680f16515a7ace9d39236464328a37743ffc28f4ddb324f4"
	   "d0f5bbdc270c65b1749a6efff1fbaa09536175ccd29fb9e6057b307320d31683"
	   "8a9c71f70b5b5907a66f7ea49aadc409");
	auto const key
	  (QByteArray::fromHex("808182838485868788898"
			       "a8b8c8d8e8f9091929394"
			       "95969798999a9b9c9d9e9f"));
	auto const nonce
	  (QByteArray::fromHex("404142434445464748494a4b"
			       "4c4d4e4f5051525354555658"));
	auto const plaintext
	  (QByteArray::fromHex("5468652064686f6c65202870726f6e6f756"
			       "e6365642022646f6c65222920697320616c"
			       "736f206b6e6f776e2061732074686520417"
			       "369617469632077696c6420646f672c2072"
			       "656420646f672c20616e642077686973746"
			       "c696e6720646f672e204974206973206162"
			       "6f7574207468652073697a65206f6620612"
			       "04765726d616e2073686570686572642062"
			       "7574206c6f6f6b73206d6f7265206c696b6"
			       "52061206c6f6e672d6c656767656420666f"
			       "782e205468697320686967686c7920656c7"
			       "57369766520616e6420736b696c6c656420"
			       "6a756d70657220697320636c61737369666"
			       "96564207769746820776f6c7665732c2063"
			       "6f796f7465732c206a61636b616c732c206"
			       "16e6420666f78657320696e207468652074"
			       "61786f6e6f6d69632066616d696c7920436"
			       "16e696461652e"));
	const uint32_t counter = 1;

	qDebug() << "XChaCha20 Encrypt Computed: "
		 << dooble_xchacha20::xchacha20_encrypt(key,
							nonce,
							plaintext,
							counter).toHex()
		 << ".";
	qDebug() << "XChaCha20 Encrypt Expected: "
		 << result
		 << ".";
      }

      {
	QByteArray ciphertext;
	auto const
	  plaintext(QByteArray("If you love Dooble, say so. Creepy."));
	dooble_xchacha20 xchacha20("This is a wonderful key.");

	ciphertext = xchacha20.encrypt(plaintext);
	qDebug() << "XChaCha20 Encrypt: "
		 << ciphertext.toHex()
		 << ".";
	qDebug() << "XChaCha20 Decrypt: "
		 << xchacha20.decrypt(ciphertext)
		 << ".";
      }
    }

#ifdef Q_OS_MACOS
  struct rlimit rlim = {0, 0};

  getrlimit(RLIMIT_NOFILE, &rlim);
  rlim.rlim_cur = OPEN_MAX;
  setrlimit(RLIMIT_NOFILE, &rlim);
#endif

  QList<int> list;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
  struct sigaction signal_action = {};
#endif

  list << SIGABRT
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
       << SIGBUS
#endif
       << SIGFPE
       << SIGILL
       << SIGINT
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
       << SIGQUIT
#endif
       << SIGSEGV
       << SIGTERM;

  foreach(auto const i, list)
    {
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
      memset(&signal_action, 0, sizeof(struct sigaction));
      signal_action.sa_handler = signal_handler;
      sigemptyset(&signal_action.sa_mask);
      signal_action.sa_flags = 0;

      if(sigaction(i,
		   &signal_action,
		   static_cast<struct sigaction *> (nullptr)) != 0)
	std::cerr << "sigaction() failure on " << i << std::endl;
#else
      signal(i, signal_handler);
#endif
    }

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_UNIX)
  /*
  ** Ignore SIGPIPE.
  */

  memset(&signal_action, 0, sizeof(struct sigaction));
  signal_action.sa_handler = SIG_IGN;
  sigemptyset(&signal_action.sa_mask);
  signal_action.sa_flags = 0;

  if(sigaction(SIGPIPE,
	       &signal_action,
	       static_cast<struct sigaction *> (nullptr)) != 0)
    std::cerr << "sigaction() failure on SIGPIPE" << std::endl;
#endif
  qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>
    ("QAbstractItemModel::LayoutChangeHint");
  qRegisterMetaType<QList<QPersistentModelIndex> >
    ("QListQPersistentModelIndex");
  qRegisterMetaType<QListPairIconString> ("QListPairIconString");
  qRegisterMetaType<QListUrl> ("QListUrl");
  qRegisterMetaType<QListVectorByteArray> ("QListVectorByteArray");
  qRegisterMetaType<QVector<qreal> > ("QVector<qreal>");
  qRegisterMetaType<Qt::SortOrder> ("Qt::SortOrder");
  qRegisterMetaType<dooble_charts::Properties> ("dooble_charts::Properties");
#if defined(Q_OS_MACOS) || defined(Q_OS_WINDOWS)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
#ifdef Q_OS_WINDOWS
  QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
#endif
#else
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QCoreApplication::setAttribute
    (Qt::AA_EnableHighDpiScaling,
     QVariant(qEnvironmentVariable("AA_ENABLEHIGHDPISCALING")).toBool());
  QCoreApplication::setAttribute
    (Qt::AA_UseHighDpiPixmaps,
     QVariant(qEnvironmentVariable("AA_USEHIGHDPIPIXMAPS")).toBool());
#endif
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
#ifdef DOOBLE_REGISTER_GOPHER_SCHEME
  QWebEngineUrlScheme scheme("gopher");

  scheme.setDefaultPort(70);
  scheme.setFlags(QWebEngineUrlScheme::ViewSourceAllowed);
  scheme.setSyntax(QWebEngineUrlScheme::Syntax::HostAndPort);
  QWebEngineUrlScheme::registerScheme(scheme);
#endif
#ifdef DOOBLE_REGISTER_JAR_SCHEME
  QWebEngineUrlScheme scheme("jar");

  scheme.setFlags(QWebEngineUrlScheme::ViewSourceAllowed);
  scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
  QWebEngineUrlScheme::registerScheme(scheme);
#endif
#endif
  QString dooble_settings_path("");
  dooble::s_application = new dooble_application(argc, argv);

#if defined(Q_OS_WINDOWS)
  auto const bytes(qEnvironmentVariable("DOOBLE_HOME").trimmed());

  if(bytes.isEmpty())
    {
      QFileInfo file_info;
      QString dooble_directory(".dooble");
      auto const username
	(qEnvironmentVariable("USERNAME").mid(0, 128).trimmed());
      auto home_directory(QDir::current());

      file_info = QFileInfo(home_directory.absolutePath());

      if(!(file_info.isReadable() && file_info.isWritable()))
	home_directory = QDir::home();

      if(username.isEmpty())
	home_directory.mkdir(dooble_directory);
      else
	home_directory.mkpath(username + QDir::separator() + dooble_directory);

      if(username.isEmpty())
	dooble_settings::set_setting
	  ("home_path",
	   dooble_settings_path =
	   home_directory.absolutePath() +
	   QDir::separator() +
	   dooble_directory);
      else
	dooble_settings::set_setting
	  ("home_path",
	   dooble_settings_path =
	   home_directory.absolutePath() +
	   QDir::separator() +
	   username +
	   QDir::separator() +
	   dooble_directory);
    }
  else
    {
      QDir().mkpath(bytes);

      dooble_settings::set_setting("home_path", dooble_settings_path = bytes);
    }
#else
  auto const bytes(qEnvironmentVariable("DOOBLE_HOME").trimmed());

  if(bytes.isEmpty())
    {
      auto const xdg_config_home
	(qEnvironmentVariable("XDG_CONFIG_HOME").trimmed());
      auto const xdg_data_home
	(qEnvironmentVariable("XDG_DATA_HOME").trimmed());

      if(xdg_config_home.isEmpty() && xdg_data_home.isEmpty())
	{
	  QString dooble_directory(".dooble");
	  auto home_directory(QDir::home());

	  home_directory.mkdir(dooble_directory);
	  dooble_settings::set_setting
	    ("home_path",
	     dooble_settings_path =
	     home_directory.absolutePath() +
	     QDir::separator() +
	     dooble_directory);
	}
      else
	{
	  QDir home_directory;

	  if(!xdg_config_home.isEmpty())
	    home_directory = QDir(xdg_config_home);
	  else
	    home_directory = QDir(xdg_data_home);

	  home_directory.mkdir("dooble");
	  dooble_settings::set_setting
	    ("home_path",
	     dooble_settings_path =
	     home_directory.absolutePath() + QDir::separator() + "dooble");
	}
    }
  else
    {
      QDir().mkpath(bytes);

      dooble_settings::set_setting("home_path", dooble_settings_path = bytes);
    }
#endif

  dooble::s_application->install_translator();
  dooble_settings::prepare_web_engine_environment_variables();
  dooble::s_default_web_engine_profile = new QWebEngineProfile("Dooble");
#if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 0))
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::ReadingFromCanvasEnabled,
     dooble_settings::reading_from_canvas_enabled());
#endif
  dooble::s_default_http_user_agent = dooble::s_default_web_engine_profile->
    httpUserAgent();
  dooble::s_settings = new dooble_settings();
  dooble::s_settings->set_settings_path(dooble_settings_path);

  /*
  ** Create a splash screen.
  */

  QSplashScreen splash;

  dooble::s_settings->prepare_application_fonts();

  auto const splash_screen = dooble_settings::setting
    ("splash_screen", true).toBool();

  if(splash_screen)
    {
      splash.setEnabled(false);
      splash.setPixmap(QPixmap(":/Miscellaneous/splash.png"));
      splash.show();
      splash.showMessage
	(QObject::tr("Initializing Dooble's random number generator."),
	 Qt::AlignBottom | Qt::AlignHCenter,
	 QColor(Qt::white));
      splash.repaint();
      dooble::s_application->processEvents();
    }

  dooble_random::initialize();

  if(splash_screen)
    dooble::s_application->processEvents();

#ifdef Q_OS_MACOS
  /*
  ** Eliminate pool errors on MacOS.
  */

  CocoaInitializer cocoa_initializer;
#endif

  if(splash_screen)
    {
      splash.showMessage
	(QObject::tr("Purging temporary database entries."),
	 Qt::AlignBottom | Qt::AlignHCenter,
	 QColor(Qt::white));
      splash.repaint();
      dooble::s_application->processEvents();
    }

  dooble_certificate_exceptions_menu_widget::purge_temporary();
  dooble_favicons::purge_temporary();

  if(splash_screen)
    {
      splash.showMessage
	(QObject::tr("Preparing QWebEngine."),
	 Qt::AlignBottom | Qt::AlignHCenter,
	 QColor(Qt::white));
      splash.repaint();
      dooble::s_application->processEvents();
    }

  dooble::s_default_web_engine_profile->setCachePath
    (dooble_settings::setting("home_path").toString() +
     QDir::separator() +
     "WebEngineCache");
  dooble::s_default_web_engine_profile->setHttpCacheMaximumSize(0);
  dooble::s_default_web_engine_profile->setHttpCacheType
    (QWebEngineProfile::MemoryHttpCache);
  dooble::s_default_web_engine_profile->setPersistentCookiesPolicy
    (QWebEngineProfile::NoPersistentCookies);
  dooble::s_default_web_engine_profile->setPersistentStoragePath
    (dooble_settings::setting("home_path").toString() +
     QDir::separator() +
     "WebEnginePersistentStorage");
  dooble::s_default_web_engine_profile->setSpellCheckEnabled(true);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ErrorPageEnabled, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::FocusOnNavigationEnabled, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::FullScreenSupportEnabled, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::JavascriptCanOpenWindows, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::LocalContentCanAccessFileUrls, false);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::LocalStorageEnabled, true);
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::ScreenCaptureEnabled, false);
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  QWebEngineSettings::defaultSettings()->setAttribute
    (QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
#endif
#else
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::ErrorPageEnabled, true);
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::FocusOnNavigationEnabled, true);
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::FullScreenSupportEnabled, true);
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::JavascriptCanOpenWindows, true);
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::LocalContentCanAccessFileUrls, false);
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::ScreenCaptureEnabled, false);
#ifndef DOOBLE_FREEBSD_WEBENGINE_MISMATCH
  dooble::s_default_web_engine_profile->settings()->setAttribute
    (QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
#endif
#endif

  if(splash_screen)
    {
      splash.showMessage(QObject::tr("Preparing Dooble objects."),
			 Qt::AlignBottom | Qt::AlignHCenter,
			 QColor(Qt::white));
      splash.repaint();
      dooble::s_application->processEvents();
    }

  auto const arguments(QCoreApplication::arguments());
  auto d = new dooble // Deleted during exit.
    (urls,
     attach,
     disable_javascript,
     false, // Pinned
     arguments.contains("--private") || dooble::s_settings->
                                        setting("private_mode").toBool(),
     reload_periodically);

  if(attach && d->attached())
    {
      d->close();
      return EXIT_SUCCESS;
    }

  dooble::s_google_translate_url = qEnvironmentVariable
    ("DOOBLE_GOOGLE_TRANSLATE_URL").trimmed().mid(0, 1024).trimmed();
  dooble::s_google_translate_url = dooble::s_google_translate_url.isEmpty() ?
    "https://%1.translate.goog/"
    "%2?_x_tr_sl=auto&_x_tr_tl=%3&_x_tr_hl=%3&_x_tr_pto=wapp" :
    dooble::s_google_translate_url;
  dooble::s_settings->prepare_application_fonts();
  QObject::connect(dooble::s_default_web_engine_profile->cookieStore(),
		   SIGNAL(cookieAdded(const QNetworkCookie &)),
		   dooble::s_cookies,
		   SLOT(slot_cookie_added(const QNetworkCookie &)));
  QObject::connect(dooble::s_default_web_engine_profile->cookieStore(),
		   SIGNAL(cookieRemoved(const QNetworkCookie &)),
		   dooble::s_cookies,
		   SLOT(slot_cookie_removed(const QNetworkCookie &)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_accepted_or_blocked_domains,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_certificate_exceptions,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_cookies,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_downloads,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_history,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_search_engines_window,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_settings,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_style_sheet,
		   SLOT(slot_populate(void)));
  QObject::connect(dooble::s_application,
		   SIGNAL(address_widget_populated(void)),
		   d,
		   SLOT(slot_populated(void)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(cookies_added(const QList<QNetworkCookie> &,
					const QList<int> &)),
		   dooble::s_cookies_window,
		   SLOT(slot_cookies_added(const QList<QNetworkCookie> &,
					   const QList<int> &)));
  QObject::connect(dooble::s_cookies,
		   SIGNAL(cookie_removed(const QNetworkCookie &)),
		   dooble::s_cookies_window,
		   SLOT(slot_cookie_removed(const QNetworkCookie &)));
  QObject::connect(dooble::s_cookies_window,
		   SIGNAL(delete_cookie(const QNetworkCookie &)),
		   dooble::s_cookies,
		   SLOT(slot_delete_cookie(const QNetworkCookie &)));
  QObject::connect(dooble::s_cookies_window,
		   SIGNAL(delete_domain(const QString &)),
		   dooble::s_cookies,
		   SLOT(slot_delete_domain(const QString &)));
  QObject::connect(dooble::s_cookies_window,
		   SIGNAL(delete_items(const QList<QNetworkCookie> &,
				       const QStringList &)),
		   dooble::s_cookies,
		   SLOT(slot_delete_items(const QList<QNetworkCookie> &,
					  const QStringList &)));
  QObject::connect(dooble::s_settings,
		   SIGNAL(dooble_credentials_authenticated(bool)),
		   dooble::s_application,
		   SIGNAL(dooble_credentials_authenticated(bool)));
  QObject::connect(dooble::s_settings,
		   SIGNAL(populated(void)),
		   d,
		   SLOT(slot_populated(void)));

  if(splash_screen)
    {
      splash.showMessage(QObject::tr("Populating Dooble containers."),
			 Qt::AlignBottom | Qt::AlignHCenter,
			 QColor(Qt::white));
      splash.repaint();
      dooble::s_application->processEvents();

      while(!d->initialized())
	splash.repaint();

      splash.showMessage(QObject::tr("Opening Dooble."),
			 Qt::AlignBottom | Qt::AlignHCenter,
			 QColor(Qt::white));
      splash.repaint();
      dooble::s_application->processEvents();
      splash.finish(d);
    }

  if(screen_mode == "full-screen")
    QTimer::singleShot(0, d, SLOT(showFullScreen(void)));
  else if(screen_mode == "normal-screen")
    QTimer::singleShot(0, d, SLOT(showNormal(void)));
  else
    QTimer::singleShot(0, d, SLOT(show(void)));

  auto const rc = dooble::s_application->exec();

  dooble::clean();
  return static_cast<int> (rc);
}
