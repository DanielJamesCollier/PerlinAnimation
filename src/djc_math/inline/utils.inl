namespace djc::math {

//------------------------------------------------------------
constexpr float
normalise(float valToNormalise, float min, float max) {
    return (valToNormalise - min) / (max - min);
}

} // namespace djc::math 

