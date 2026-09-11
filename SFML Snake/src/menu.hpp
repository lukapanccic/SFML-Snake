#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

void centerOrigin(sf::Text& text);

class Menu {
	std::vector<std::string> items;
	int selectedIndex;

	std::vector<sf::FloatRect> computeLayout(sf::RenderWindow& window) const;

public:
	explicit Menu(std::vector<std::string> items);

	void moveUp();
	void moveDown();
	int getSelectedIndex() const;

	void draw(sf::RenderWindow& window, const sf::Font& font, const std::string& title = "");
};
