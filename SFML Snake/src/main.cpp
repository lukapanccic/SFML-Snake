#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <string>
#include "game.hpp"
#include "menu.hpp"
#include "multiplayer.hpp"
#include "Protocol.hpp"

enum class AppState
{
	MainMenu,
	UsernameEntry,
	Lobby,
	InviteSent,
	InviteReceived,
	Room,
	Playing,
	Paused,
	GameOver
};

int main()
{
	sf::RenderWindow window(sf::VideoMode({ 600, 600 }), "Snake");
	window.setFramerateLimit(30);

	sf::Font font;
	if (!font.openFromFile("C:/Windows/Fonts/arial.ttf"))
	{
		std::cerr << "Failed to load font\n";
		return 1;
	}

	Menu mainMenu({ "Single Player", "Multiplayer", "Exit" });
	Menu pauseMenu({ "Resume", "Main Menu", "Exit" });

	AppState state = AppState::MainMenu;

	std::string usernameInput;
	MultiplayerClient mpClient;
	bool showConnectError = false;
	sf::Clock messageClock;

	int selectedPlayerIndex = 0;
	std::string seedInput;

	std::string toastMessage;
	sf::Clock toastClock;
	bool showToast = false;

	// seed
	std::mt19937 random(std::random_device{}());
	Game g(random);
	sf::Clock clock;

	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event->is<sf::Event::Closed>())
				window.close();

			if (auto keyEvent = event->getIf<sf::Event::KeyPressed>())
			{
				sf::Keyboard::Key key = keyEvent->code;

				if (state == AppState::MainMenu)
				{
					if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W)
						mainMenu.moveUp();
					else if (key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S)
						mainMenu.moveDown();
					else if (key == sf::Keyboard::Key::Enter)
					{
						int selected = mainMenu.getSelectedIndex();
						if (selected == 0)
						{
							g = Game(std::mt19937(std::random_device{}()));
							clock.restart();
							state = AppState::Playing;
						}
						else if (selected == 1)
						{
							usernameInput.clear();
							showConnectError = false;
							state = AppState::UsernameEntry;
						}
						else if (selected == 2)
							window.close();
					}
				}
				else if (state == AppState::UsernameEntry)
				{
					if (key == sf::Keyboard::Key::Escape)
						state = AppState::MainMenu;
					else if (key == sf::Keyboard::Key::Backspace)
					{
						if (!usernameInput.empty())
							usernameInput.pop_back();
					}
					else if (key == sf::Keyboard::Key::Enter)
					{
						if (!usernameInput.empty())
						{
							if (mpClient.connect(Protocol::ServerAddress, Protocol::PORT, usernameInput))
							{
								selectedPlayerIndex = 0;
								state = AppState::Lobby;
							}
							else
							{
								showConnectError = true;
								messageClock.restart();
							}
						}
					}
				}
				else if (state == AppState::Lobby)
				{
					const std::vector<std::string>& players = mpClient.getPlayers();
					if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W)
					{
						if (!players.empty())
							selectedPlayerIndex = (selectedPlayerIndex - 1 + (int)players.size()) % (int)players.size();
					}
					else if (key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S)
					{
						if (!players.empty())
							selectedPlayerIndex = (selectedPlayerIndex + 1) % (int)players.size();
					}
					else if (key == sf::Keyboard::Key::Enter)
					{
						if (selectedPlayerIndex >= 0 && selectedPlayerIndex < (int)players.size())
							mpClient.invitePlayer(players[selectedPlayerIndex]);
					}
					else if (key == sf::Keyboard::Key::Escape)
					{
						mpClient.disconnect();
						state = AppState::MainMenu;
					}
				}
				else if (state == AppState::InviteSent)
				{
					if (key == sf::Keyboard::Key::Escape)
						mpClient.cancelInvite();
				}
				else if (state == AppState::InviteReceived)
				{
					if (key == sf::Keyboard::Key::Enter)
						mpClient.acceptInvite();
					else if (key == sf::Keyboard::Key::Escape)
						mpClient.declineInvite();
				}
				else if (state == AppState::Room)
				{
					if (key == sf::Keyboard::Key::Escape)
						mpClient.leaveRoom();
					else if (mpClient.isRoomMaster())
					{
						if (key == sf::Keyboard::Key::Backspace)
						{
							if (!seedInput.empty())
								seedInput.pop_back();
						}
						else if (key == sf::Keyboard::Key::Enter)
						{
							if (!seedInput.empty())
							{
								mpClient.setSeed(static_cast<std::uint32_t>(std::stoul(seedInput)));
								seedInput.clear();
							}
						}
						else if (key == sf::Keyboard::Key::R)
						{
							mpClient.randomizeSeed();
							seedInput.clear();
						}
						else if (key == sf::Keyboard::Key::Space)
							mpClient.startGame();
					}
				}
				else if (state == AppState::Playing)
				{
					if (key == sf::Keyboard::Key::Escape)
						state = AppState::Paused;
					else
						g.handleInput(key);
				}
				else if (state == AppState::Paused)
				{
					if (key == sf::Keyboard::Key::Escape)
					{
						state = AppState::Playing;
						clock.restart();
					}
					else if (key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::W)
						pauseMenu.moveUp();
					else if (key == sf::Keyboard::Key::Down || key == sf::Keyboard::Key::S)
						pauseMenu.moveDown();
					else if (key == sf::Keyboard::Key::Enter)
					{
						int selected = pauseMenu.getSelectedIndex();
						if (selected == 0)
						{
							state = AppState::Playing;
							clock.restart();
						}
						else if (selected == 1)
							state = AppState::MainMenu;
						else if (selected == 2)
							window.close();
					}
				}
				else if (state == AppState::GameOver)
				{
					if (key == sf::Keyboard::Key::Enter)
						state = AppState::MainMenu;
				}
			}

			if (state == AppState::UsernameEntry)
			{
				if (auto textEvent = event->getIf<sf::Event::TextEntered>())
				{
					char32_t unicode = textEvent->unicode;
					if (unicode >= 32 && unicode < 127 && usernameInput.size() < 20)
						usernameInput += static_cast<char>(unicode);
				}
			}
			else if (state == AppState::Room && mpClient.isRoomMaster())
			{
				if (auto textEvent = event->getIf<sf::Event::TextEntered>())
				{
					char32_t unicode = textEvent->unicode;
					if (unicode >= '0' && unicode <= '9' && seedInput.size() < 9)
						seedInput += static_cast<char>(unicode);
				}
			}
		}

		bool inNetworkedLobby = state == AppState::Lobby || state == AppState::InviteSent ||
			state == AppState::InviteReceived || state == AppState::Room;

		if (inNetworkedLobby)
		{
			mpClient.update();

			if (!mpClient.isConnected())
			{
				state = AppState::MainMenu;
			}
			else
			{
				switch (mpClient.getStatus())
				{
				case LobbyStatus::Idle:
					state = AppState::Lobby;
					seedInput.clear();
					break;
				case LobbyStatus::InviteSent:
					state = AppState::InviteSent;
					seedInput.clear();
					break;
				case LobbyStatus::InviteReceived:
					state = AppState::InviteReceived;
					seedInput.clear();
					break;
				case LobbyStatus::InRoom:
					state = AppState::Room;
					break;
				}

				if (mpClient.hasNotice())
				{
					toastMessage = mpClient.takeNotice();
					showToast = true;
					toastClock.restart();
				}

				std::uint32_t startedSeed = 0;
				if (mpClient.consumeGameStarted(startedSeed))
				{
					g = Game(std::mt19937(startedSeed));
					clock.restart();
					state = AppState::Playing;
				}
			}
		}

		if (state == AppState::Playing)
		{
			if (g.isGameOver())
			{
				state = AppState::GameOver;
			}
			else if (clock.getElapsedTime().asSeconds() >= 0.2f)
			{
				g.update();

				clock.restart();
			}
		}

		window.clear();

		if (state == AppState::MainMenu)
		{
			mainMenu.draw(window, font, "Snake");

			if (showConnectError)
			{
				if (messageClock.getElapsedTime().asSeconds() < 2.5f)
				{
					sf::Text message(font, mpClient.getLastError(), 20);
					message.setFillColor(sf::Color::Red);
					centerOrigin(message);
					message.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y - 40.f));
					window.draw(message);
				}
				else
				{
					showConnectError = false;
				}
			}
		}
		else if (state == AppState::UsernameEntry)
		{
			sf::Text title(font, "Enter your username", 32);
			title.setFillColor(sf::Color::White);
			centerOrigin(title);
			title.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f - 60.f));
			window.draw(title);

			sf::RectangleShape box(sf::Vector2f(280.f, 44.f));
			box.setOrigin(sf::Vector2f(140.f, 22.f));
			box.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f));
			box.setFillColor(sf::Color(60, 60, 60));
			box.setOutlineThickness(2.f);
			box.setOutlineColor(sf::Color::White);
			window.draw(box);

			std::string display = usernameInput;
			if (static_cast<int>(messageClock.getElapsedTime().asSeconds() * 2.f) % 2 == 0)
				display += "_";

			sf::Text input(font, display, 24);
			input.setFillColor(sf::Color::White);
			centerOrigin(input);
			input.setPosition(box.getPosition());
			window.draw(input);

			sf::Text hint(font, "Enter - connect | Escape - cancel", 16);
			hint.setFillColor(sf::Color(200, 200, 200));
			centerOrigin(hint);
			hint.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f + 60.f));
			window.draw(hint);
		}
		else if (state == AppState::Lobby)
		{
			sf::Text title(font, "Lobby", 32);
			title.setFillColor(sf::Color::White);
			centerOrigin(title);
			title.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 60.f));
			window.draw(title);

			const std::vector<std::string>& players = mpClient.getPlayers();
			for (size_t i = 0; i < players.size(); ++i)
			{
				bool isSelf = players[i] == usernameInput;
				bool isSelected = (int)i == selectedPlayerIndex;

				if (isSelected)
				{
					sf::RectangleShape highlight(sf::Vector2f(240.f, 30.f));
					highlight.setOrigin(sf::Vector2f(120.f, 15.f));
					highlight.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 120.f + i * 34.f));
					highlight.setFillColor(sf::Color(80, 80, 80));
					highlight.setOutlineThickness(2.f);
					highlight.setOutlineColor(sf::Color::Yellow);
					window.draw(highlight);
				}

				sf::Text playerText(font, players[i], 22);
				playerText.setFillColor(isSelf ? sf::Color::Yellow : sf::Color::White);
				centerOrigin(playerText);
				playerText.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 120.f + i * 34.f));
				window.draw(playerText);
			}

			sf::Text hint(font, "Up/Down - select | Enter - invite | Escape - leave", 16);
			hint.setFillColor(sf::Color(200, 200, 200));
			centerOrigin(hint);
			hint.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y - 30.f));
			window.draw(hint);
		}
		else if (state == AppState::InviteSent)
		{
			sf::Text title(font, "Waiting for " + mpClient.getInvitePeer() + "...", 26);
			title.setFillColor(sf::Color::White);
			centerOrigin(title);
			title.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f - 30.f));
			window.draw(title);

			sf::Text countdown(font, std::to_string((int)mpClient.getInviteRemainingSeconds()) + "s remaining", 20);
			countdown.setFillColor(sf::Color(200, 200, 200));
			centerOrigin(countdown);
			countdown.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f + 10.f));
			window.draw(countdown);

			sf::Text hint(font, "Escape - cancel invite", 16);
			hint.setFillColor(sf::Color(200, 200, 200));
			centerOrigin(hint);
			hint.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f + 60.f));
			window.draw(hint);
		}
		else if (state == AppState::InviteReceived)
		{
			sf::Text title(font, mpClient.getInvitePeer() + " invited you to play!", 24);
			title.setFillColor(sf::Color::White);
			centerOrigin(title);
			title.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f - 30.f));
			window.draw(title);

			sf::Text countdown(font, std::to_string((int)mpClient.getInviteRemainingSeconds()) + "s to respond", 20);
			countdown.setFillColor(sf::Color(200, 200, 200));
			centerOrigin(countdown);
			countdown.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f + 10.f));
			window.draw(countdown);

			sf::Text hint(font, "Enter - accept | Escape - decline", 16);
			hint.setFillColor(sf::Color(200, 200, 200));
			centerOrigin(hint);
			hint.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f + 60.f));
			window.draw(hint);
		}
		else if (state == AppState::Room)
		{
			sf::Text title(font, "Match vs " + mpClient.getOpponentUsername(), 28);
			title.setFillColor(sf::Color::White);
			centerOrigin(title);
			title.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 80.f));
			window.draw(title);

			sf::Text seedText(font, "Seed: " + std::to_string(mpClient.getRoomSeed()), 22);
			seedText.setFillColor(sf::Color::White);
			centerOrigin(seedText);
			seedText.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 150.f));
			window.draw(seedText);

			if (mpClient.isRoomMaster())
			{
				sf::Text roleText(font, "You are the room master", 18);
				roleText.setFillColor(sf::Color::Yellow);
				centerOrigin(roleText);
				roleText.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 190.f));
				window.draw(roleText);

				sf::Text seedInputText(font, "New seed: " + seedInput, 20);
				seedInputText.setFillColor(sf::Color(200, 200, 200));
				centerOrigin(seedInputText);
				seedInputText.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 240.f));
				window.draw(seedInputText);

				sf::Text hint(font, "Type digits + Enter - set seed | R - random | Space - start | Escape - leave", 14);
				hint.setFillColor(sf::Color(200, 200, 200));
				centerOrigin(hint);
				hint.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y - 30.f));
				window.draw(hint);
			}
			else
			{
				sf::Text waitingText(font, "Waiting for the host to start...", 20);
				waitingText.setFillColor(sf::Color(200, 200, 200));
				centerOrigin(waitingText);
				waitingText.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 220.f));
				window.draw(waitingText);

				sf::Text hint(font, "Escape - leave", 16);
				hint.setFillColor(sf::Color(200, 200, 200));
				centerOrigin(hint);
				hint.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y - 30.f));
				window.draw(hint);
			}
		}
		else if (state == AppState::Playing)
		{
			g.draw(window, font);
		}
		else if (state == AppState::Paused)
		{
			g.draw(window, font);

			sf::RectangleShape overlay(sf::Vector2f((float)window.getSize().x, (float)window.getSize().y));
			overlay.setFillColor(sf::Color(0, 0, 0, 150));
			window.draw(overlay);

			pauseMenu.draw(window, font, "Paused");
		}
		else if (state == AppState::GameOver)
		{
			g.draw(window, font);

			sf::RectangleShape overlay(sf::Vector2f((float)window.getSize().x, (float)window.getSize().y));
			overlay.setFillColor(sf::Color(0, 0, 0, 150));
			window.draw(overlay);

			sf::Text overText(font, "Game Over - Score: " + std::to_string(g.getScore()), 28);
			overText.setFillColor(sf::Color::White);
			centerOrigin(overText);
			overText.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f - 20.f));
			window.draw(overText);

			sf::Text hint(font, "Enter - Main Menu", 18);
			hint.setFillColor(sf::Color(200, 200, 200));
			centerOrigin(hint);
			hint.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f + 20.f));
			window.draw(hint);
		}

		if (showToast)
		{
			if (inNetworkedLobby && toastClock.getElapsedTime().asSeconds() < 2.5f)
			{
				sf::Text toast(font, toastMessage, 18);
				toast.setFillColor(sf::Color::Red);
				centerOrigin(toast);
				toast.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y - 60.f));
				window.draw(toast);
			}
			else
			{
				showToast = false;
			}
		}

		window.display();
	}
	return 0;
}
