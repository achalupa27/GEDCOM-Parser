#ifndef GEDCOMUTILITIES_H
#define GEDCOMUTILITIES_H
#include "GEDCOMparser.h"

typedef struct storeIndividual {
    char id[32];
    Individual *individual;
    int size;
} s_indiv;

typedef struct storeFamily {
    char id[32];
    Family *family;
} s_fam;

char *errorType(GEDCOMerror err);
Event *addEvent(char *type, char *date, char *place);
Field *addField(char *tag, char *value);
void addName(s_indiv *individual, char *name);
Individual *createIndividual();
Family *createFamily();
CharSet convertCharSet(char *encoding);
void checkFile(FILE *file);
bool compareFind(const void *first, const void *second);
void deleteDummy(void *toBeDeleted);
char *printDummy(void *toBePrinted);
int compareDummy(const void *first, const void *second);
int getMonthValue(char *month);
char *encodeTypeToStr(CharSet encoding);
char *parseLine(char *token);
Submitter *createTestSubmitter(char *name, char *address);
Header *createTestHeader(char *source, float gedcVersion, CharSet encoding, Submitter *headerSubmitter);
Event *createTestEvent(char *date, char *place, char *type);
Field *createTestField(char *tag, char *value);
Individual *createTestIndividual(char *givenName, char *surname);
Family *createTestFamily(Individual *husband, Individual *wife);
GEDCOMobject *createTestObject();

#endif
