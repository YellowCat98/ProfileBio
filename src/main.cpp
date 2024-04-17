#ifdef __APPLE__

/*
	silly goofy fix because it errors if it comes after geode includes
	than you prevter and ninxout
	- raydeeux
*/

#define CommentType CommentTypeDummy
#include <CoreGraphics/CoreGraphics.h>
#include <CoreServices/CoreServices.h>
#undef CommentType

#endif
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/utils/web.hpp>
#include <matjson.hpp>
#include <imgui.h>
#include <imgui-cocos.hpp>
#include <iostream>
#include <sstream>
#include <cctype>
#include <iomanip>
#include <string>

using namespace geode::prelude;

std::string urlEncode(const std::string& str) {
	std::ostringstream encoded;
	encoded << std::hex << std::uppercase;

	for (char ch : str) {
		if (std::isalnum(ch) || ch == '-' || ch == '.' || ch == '~' || ch == '\n') {
			if (ch == '\n') {
				encoded << "%0A";
			} else {
				encoded << ch;
			}
		} else {
			encoded << '%' << std::setw(2) << int(static_cast<unsigned char>(ch));
		}
	}
	return encoded.str();
}

class $modify(BioProfilePage, ProfilePage) {
private:
	int m_accountID;

public:
	bool init(int accountID, bool ownProfile) {
		if (!ProfilePage::init(accountID, ownProfile)) { return false; }

		m_accountID = accountID;

		return true;
	}

	void fetchBioData(CCObject* pSender) {

		int accountID = m_accountID;

		auto loadingPopup = LoadingCircle::create();
		loadingPopup->show();

		web::AsyncWebRequest()
			.fetch("https://yellowcat98.5v.pl/profilebio/PB_getProfileBio.php?accountID=" + std::to_string(accountID))
			.text()
			.then([=](std::string const& bioData) {
			try {
				if (bioData == "-1") {
					FLAlertLayer::create("Error", "this person doesn't seem to have a bio attached to their profile...", "oki")->show();
					loadingPopup->fadeAndRemove();
				} else {

				auto bioObject = matjson::parse(bioData);

				auto bioAccountID = bioObject["accountID"].as_string();
				auto bioBio = bioObject["bio"].as_string();
				auto bioID = bioObject["id"].as_string();


				auto showBioPopup = FLAlertLayer::create("About me", gd::string(bioBio.c_str()), "ok");
				showBioPopup->m_scene = this;
				showBioPopup->show();

				loadingPopup->fadeAndRemove();
				}
			} catch (const std::exception& e) {
				auto errorPopup = FLAlertLayer::create("Error", "Failed to parse Json. Error: " + std::string(e.what()) + "\nThis is probably not your fault, submit a github issue if you keep encountering this error.", "Oki");
				errorPopup->m_scene = this;
				errorPopup->show();
				loadingPopup->fadeAndRemove();
			}
				
			})
			.expect([=](std::string const& error) {


				auto errorPopup = FLAlertLayer::create("Error", "Failed to fetch bio data.", "Ok");
				errorPopup->m_scene = this;
				errorPopup->show();


				loadingPopup->fadeAndRemove();

			});
	}
};


class AboutMeHandler : public cocos2d::CCObject {
	char buffer[1024] = "";
	bool theWindowShow = true;
	public:
	void onAboutMe(CCObject* pSender) {
		theWindowShow = true;

		bool& localWindowShow = theWindowShow;
			ImGuiCocos::get().setup([]{
				auto* arialFont = ImGui::GetIO().Fonts->AddFontFromFileTTF((Mod::get()->getResourcesDir() / "arial.ttf").string().c_str(), 16.0f);

				ImGui::GetIO().FontDefault = arialFont;
			}).draw([this, &localWindowShow] {
				if (localWindowShow) {
				if (ImGui::Begin("UploadBio", &localWindowShow, ImGuiWindowFlags_AlwaysAutoResize)) {

				if (ImGui::Button("Upload")) {

					std::string BioRequestBody = "?accountID=" + std::to_string(GJAccountManager::get()->m_accountID) + "&bio=" + urlEncode(buffer);

					web::AsyncWebRequest()
					.post("https://yellowcat98.5v.pl/profilebio/PB_uploadProfileBio.php" + BioRequestBody)
					.text()
					.then([=](std::string const& response) {
						bool showWindow = true;
						ImGuiCocos::get().setup([]{

							auto* arialFont = ImGui::GetIO().Fonts->AddFontFromFileTTF((Mod::get()->getResourcesDir() / "arial.ttf").string().c_str(), 16.0f);

							ImGui::GetIO().FontDefault = arialFont;
						}).draw([=]() mutable {

							if (showWindow) {
								if (ImGui::Begin("Server Response:", &showWindow, ImGuiWindowFlags_AlwaysAutoResize)) {
									ImGui::Text("%s", response.c_str());
								}
							}
						});


					})
					.expect([BioRequestBody](std::string const& error) {
						log::info("{}", error);
					});
				}

				
				ImGui::InputTextMultiline("##source", buffer, IM_ARRAYSIZE(buffer), ImVec2(500, 300));

				}
				ImGui::End();
			}
			});
	}
};

class $modify(PBProfilePage, ProfilePage) {
	void loadPageFromUserInfo(GJUserScore* p0) {
		ProfilePage::loadPageFromUserInfo(p0);
		if (m_mainLayer->getChildByID("buttons-menu"_spr)) { return; }
		auto menu = CCMenu::create();
		menu->setID("buttons-menu"_spr);
		menu->setPosition({0, 0});
		menu->setZOrder(INT8_MAX);
		
		auto bioShowSpr = ButtonSprite::create("Show Bio");
		bioShowSpr->setScale(.5f);
		auto bioShow = CCMenuItemSpriteExtra::create(
			bioShowSpr,
			this,
			menu_selector(BioProfilePage::fetchBioData)
		);
		bioShow->setPosition({getChildByIDRecursive("username-label")->getPositionX(), 168});
		bioShow->setAnchorPoint({.5, 1.f});
		bioShow->setID("show-bio-btn"_spr);
		
		menu->addChild(bioShow);
		
		#ifndef GEODE_IS_MOBILE
		if (m_ownProfile) {
			auto btn = CCSprite::create("btn.png"_spr);
			auto aboutMeBtn = CCMenuItemSpriteExtra::create(btn, nullptr, nullptr);
			aboutMeBtn->setEnabled(true);
			aboutMeBtn->setID("add-bio-btn"_spr);
			aboutMeBtn->setPosition({getChildByIDRecursive("background")->getContentWidth(), getChildByIDRecursive("bottom-menu")->getPositionY()});
			menu->addChild(aboutMeBtn);
	
			auto aboutMeHandler = new AboutMeHandler();
			aboutMeBtn->setTarget(aboutMeHandler, menu_selector(AboutMeHandler::onAboutMe));
		}
		#endif
		
		m_mainLayer->addChild(menu);
	}
};