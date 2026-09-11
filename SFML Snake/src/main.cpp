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
	Multiplayer,
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
								state = AppState::Multiplayer;
							else
							{
								showConnectError = true;
								messageClock.restart();
							}
						}
					}
				}
				else if (state == AppState::Multiplayer)
				{
					if (key == sf::Keyboard::Key::Escape)
					{
						mpClient.disconnect();
						state = AppState::MainMenu;
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
		}

		if (state == AppState::Multiplayer)
		{
			mpClient.update();
			if (!mpClient.isConnected())
				state = AppState::MainMenu;
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
				if (messageClock.getElapsedTime().asSeconds() < 1.5f)
				{
					sf::Text message(font, "Connection failed", 20);
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
		else if (state == AppState::Multiplayer)
		{
			sf::Text title(font, "Connected Players:", 32);
			title.setFillColor(sf::Color::White);
			centerOrigin(title);
			title.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 60.f));
			window.draw(title);

			const std::vector<std::string>& players = mpClient.getPlayers();
			for (size_t i = 0; i < players.size(); ++i)
			{
				sf::Text playerText(font, players[i], 22);
				playerText.setFillColor(players[i] == usernameInput ? sf::Color::Yellow : sf::Color::White);
				centerOrigin(playerText);
				playerText.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, 120.f + i * 34.f));
				window.draw(playerText);
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

		window.display();
	}
	return 0;
}
