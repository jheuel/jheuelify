#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

#include <boost/program_options.hpp>

#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/cURLpp.hpp>

#include <tgbot/tgbot.h>

#include "faceswapper.h"
#include "SignalHandler.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ofstream;
using std::stringstream;
using namespace TgBot;
namespace po = boost::program_options;

const string photoMimeType = "image/jpeg";

string replaceAll(std::string s, const std::string &from,
                  const std::string &to) {
  if (from.empty())
    return s;
  size_t start_pos = 0;
  while ((start_pos = s.find(from, start_pos)) != std::string::npos) {
    s.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  return s;
}

void print_msg(Message::Ptr message, string output_path) {
  stringstream out;
  out << "messageId: " << message->messageId << endl;
  out << "chatId: " << message->chat->id << endl;
  out << "chat username: " << message->chat->username << endl;
  out << "chat title: " << message->chat->title << endl;
  out << "chat first name: " << message->chat->firstName << endl;
  out << "chat last name: " << message->chat->lastName << endl;
  out << "chat type: " << (int)message->chat->type << endl;
  out << "User wrote " << message->text << endl;
  out << endl << endl << endl;
  cout << out.str();

  std::ofstream outfile;
  outfile.open(output_path + "/tgbt.log", std::ios::out | std::ios_base::app);
  outfile << out.str();
  outfile.close();
}

void download(const string &filepath, const string &outpath,
              const string &token) {
  try {
    ostringstream url;
    url << "https://api.telegram.org/file/bot" << token << "/" << filepath;
    std::ofstream outfile(outpath);
    curlpp::Cleanup cleanup;
    curlpp::Easy request;

    cout << "Downloading " << url.str() << endl;

    request.setOpt(new curlpp::options::Url(url.str()));
    request.setOpt(new curlpp::options::WriteStream(&outfile));
    request.perform();
  } catch (cURLpp::RuntimeError &e) {
    cerr << e.what() << endl;
  } catch (cURLpp::LogicError &e) {
    cerr << e.what() << endl;
  }
}

void handle_photo(Bot &bot, Message::Ptr message, FaceSwapper &swapper, string &output_path) {
  cout << "Caption was: " << message->caption << endl;
  if (message->photo.size() > 0) {
    cout << "Caption was: " << message->caption << endl;
  }

  for (auto i : message->photo) {
    cout << "resolution: " << i->width << "x" << i->height << endl;
  }

  string downloaded_img_path;
  string filepath_on_server =
      bot.getApi().getFile(message->photo.back()->fileId)->filePath;
  downloaded_img_path = output_path + replaceAll(filepath_on_server, "/", "_");
  download(filepath_on_server, downloaded_img_path, bot.getToken());

  string mId = to_string(message->messageId);
  string swapped_img_path =
      downloaded_img_path.substr(0, downloaded_img_path.length() - 4) + mId +
      "_cloned.jpg";
  cout << swapped_img_path << endl;
  bot.getApi().sendChatAction(message->chat->id, "upload_photo");
  try {
    cout << "swap" << endl;
    swapper.swap(downloaded_img_path, swapped_img_path);

    cout << "swapped" << endl;
    auto f = InputFile::fromFile(swapped_img_path, photoMimeType);
    if (f != nullptr) {
      bot.getApi().sendPhoto(message->chat->id, f);
    }
  } catch (exception &e) {
    bot.getApi().sendMessage(message->chat->id,
                             "Sorry, I could not find a face.");
    cout << e.what() << endl;
  }
}

bool process_arguments(int argc, char *argv[], string &api_key,
                       string &faces_path, string &dlib_model_path,
                       string &output_path) {
  try {
    po::options_description desc("Allowed options");
    // clang-format off
    desc.add_options()("help", "produce help message")
        ("api_key",             po::value<string>(&api_key)->required(),         "set telegram bot API key")
        ("faces_path",          po::value<string>(&faces_path)->required(),      "set path with images of faces")
        ("dlib_model_path",     po::value<string>(&dlib_model_path)->required(), "set path of the dlib model")
        ("output_path",         po::value<string>(&output_path)->required(),     "set output path");
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      cout << desc << "\n";
      return false;
    }

    output_path += "/";
  } catch (boost::program_options::error &e) {
    cerr << "Error: " << e.what() << endl;
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  string api_key;
  string faces_path;
  string dlib_model_path;
  string output_path;

  bool opts_ok = process_arguments(argc, argv, api_key, faces_path,
                                   dlib_model_path, output_path);
  if (!opts_ok) {
    return 1;
  }

  FaceSwapper swapper(dlib_model_path, faces_path, output_path);

  Bot bot(api_key);
  printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
  bot.getEvents().onCommand("start", [&bot](Message::Ptr message) {
    bot.getApi().sendMessage(message->chat->id, "Hi!");
  });

  // swap command need group chats, is annoying
  // bot.getEvents().onCommand("swap", [&bot](Message::Ptr message) {
  // TgBot::ForceReply::Ptr fr(new ForceReply);
  // fr->selective = true;
  // bot.getApi().sendMessage(message->chat->id, "Send an image, please.",
  // true,
  // message->messageId, fr, "", true);
  //});

  bot.getEvents().onAnyMessage(
      [&bot, &swapper, &output_path](Message::Ptr message) {
        print_msg(message, output_path);

        if (StringTools::startsWith(message->text, "/start")) {
          return;
        }
        if (StringTools::startsWith(message->text, "/swap")) {
          return;
        }

        if (message->photo.size() > 0) {
          handle_photo(bot, message, swapper, output_path);
        }
      });

  // handle kill signals, e.g. Ctrl+c
  SignalHandler signal_handler;

  TgLongPoll longPoll(bot);
  while (signal_handler.running()) {
    try {
      printf("Long poll started\n");
      longPoll.start();
    } catch (exception &e) {
      printf("error: %s\n", e.what());
    }
  }

  return 0;
}
