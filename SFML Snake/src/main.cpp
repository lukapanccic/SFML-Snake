#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <string>
#include "game.hpp"
#include "menu.hpp"

enum class AppState
{
	MainMenu,
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
	bool showComingSoon = false;
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
							showComingSoon = true;
							messageClock.restart();
						}
						else if (selected == 2)
						{
							window.close();
						}
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
						{
							state = AppState::MainMenu;
						}
						else if (selected == 2)
						{
							window.close();
						}
					}
				}
				else if (state == AppState::GameOver)
				{
					if (key == sf::Keyboard::Key::Enter)
						state = AppState::MainMenu;
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

			if (showComingSoon)
			{
				if (messageClock.getElapsedTime().asSeconds() < 1.5f)
				{
					sf::Text message(font, "Multiplayer - Coming soon!", 20);
					message.setFillColor(sf::Color::Yellow);
					centerOrigin(message);
					message.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y - 40.f));
					window.draw(message);
				}
				else
				{
					showComingSoon = false;
				}
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

			sf::Text hint(font, "Press Enter for Main Menu", 18);
			hint.setFillColor(sf::Color(200, 200, 200));
			centerOrigin(hint);
			hint.setPosition(sf::Vector2f((float)window.getSize().x / 2.f, (float)window.getSize().y / 2.f + 20.f));
			window.draw(hint);
		}

		window.display();
	}
	return 0;
}
