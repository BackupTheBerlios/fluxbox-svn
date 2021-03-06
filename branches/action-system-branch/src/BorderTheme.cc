#include "BorderTheme.hh"

BorderTheme::BorderTheme(FbTk::Theme &theme, const std::string &name,
                         const std::string &altname):
    m_width(theme, name + ".borderWidth", altname + ".BorderWidth"),
    m_color(theme, name + ".borderColor", altname + ".BorderColor") {
    // set default values
    *m_width = 0;
    m_color->setFromString("black", theme.screenNum());
}
