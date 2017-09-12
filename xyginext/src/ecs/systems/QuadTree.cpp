/*********************************************************************
(c) Matt Marchant 2017
http://trederia.blogspot.com

xygineXT - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include <xyginext/ecs/systems/QuadTree.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/QuadTreeItem.hpp>

#include <xyginext/util/Rectangle.hpp>

#ifdef _DEBUG_
#include <SFML/Graphics/RenderTarget.hpp>
#endif

using namespace xy;

QuadTree::QuadTree(xy::MessageBus& mb, sf::FloatRect rootArea)
    : xy::System(mb, typeid(QuadTree)),
    m_rootNode(rootArea, 0, nullptr, this)
{
    requireComponent<xy::Transform>();
    requireComponent<xy::QuadTreeItem>();

    m_queryVector.reserve(MaxNodeEntities * MaxLevels);
}

//public
void QuadTree::process(float)
{
    auto& entities = getEntities();
    for (auto& entity : entities)
    {
        auto node = entity.getComponent<xy::QuadTreeItem>().m_node;
        if(node) node->update(entity);
    }

#ifdef _DEBUG_
    m_vertices.clear();
    m_rootNode.getVertices(m_vertices);
#endif
}

std::vector<Entity> QuadTree::queryArea(sf::FloatRect area) const
{
    m_queryVector.clear();
    
    //check entities in outside set
    for (const auto& entity : m_outsideRoot)
    {
        auto rect = entity.getComponent<xy::Transform>().getWorldTransform().transformRect(entity.getComponent<xy::QuadTreeItem>().m_area);
        if (area.intersects(rect))
        {
            m_queryVector.push_back(entity);
        }
    }

    //walk the tree adding as we go
    std::vector<QuadTreeNode*> nodeList;
    nodeList.reserve(MaxLevels);
    nodeList.push_back(&m_rootNode);
    
    while (!nodeList.empty())
    {
        auto* currentNode = nodeList.back();
        nodeList.pop_back();

        if (area.intersects(currentNode->getArea()))
        {
            //add any entities in this node
            for (const auto& entity : currentNode->getEntities())
            {
                auto rect = entity.getComponent<xy::Transform>().getWorldTransform().transformRect(entity.getComponent<xy::QuadTreeItem>().m_area);
                if (area.intersects(rect))
                {
                    m_queryVector.push_back(entity);
                }
            }

            //check any child nodes
            if (currentNode->hasChildren())
            {
                const auto& children = currentNode->getChildNodes();
                for (const auto& c : children)
                {
                    if (c && c->getNumEntsBelow() > 0)
                    {
                        nodeList.push_back(c.get());
                    }
                }
            }
        }
    }

    return {};
}

sf::FloatRect QuadTree::getRootArea() const
{
    return m_rootNode.getArea();
}

const std::vector<xy::Entity>& QuadTree::getOutsideRootEnts() const
{
    return m_outsideRoot;
}

std::vector<xy::Entity>& QuadTree::getOutsideRootEnts()
{
    return m_outsideRoot;
}

std::size_t QuadTree::getEntityCount() const
{
    return m_rootNode.getEntityCount();
}

//private
void QuadTree::onEntityAdded(xy::Entity entity)
{
    auto& item = entity.getComponent<xy::QuadTreeItem>();
    item.m_quadTree = this;

    auto rect = entity.getComponent<xy::Transform>().getWorldTransform().transformRect(item.m_area);
    if (Util::Rectangle::contains(m_rootNode.getArea(), rect))
    {
        m_rootNode.addEntity(entity);
    }
    else
    {
        //falls outside
        m_outsideRoot.push_back(entity);
    }
}

void QuadTree::onEntityRemoved(xy::Entity entity)
{
    entity.getComponent<xy::QuadTreeItem>().m_node->removeEntity(entity);
}

#ifdef _DEBUG_
void QuadTree::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
    rt.draw(m_vertices.data(), m_vertices.size(), sf::Lines, states);
}
#endif